#include "serialmanager.h"
//#include <qfile.h>
#include <qdebug.h>
#include <algorithm>
#include <qtimer.h>
#include <qdatastream.h>
#include "util.h"

//Constructor
Serial::Serial(QString portName, QObject *parent) : QObject(parent), pFirmwareVersion("Not connected"), pUpdatesMissed(0)
{
    //Create the serial port
	serialPort = new QSerialPort(portName);
    pPortName = portName;
	pDeviceConnected = false;
	
	//Setup port
    serialPort->setBaudRate(1000000); //1MBaud
	serialPort->setFlowControl(QSerialPort::NoFlowControl);
	serialPort->setParity(QSerialPort::NoParity);
	serialPort->setDataBits(QSerialPort::Data8);
	serialPort->setStopBits(QSerialPort::OneStop);
	
    //Initialize port
	changeSerialPort(portName);

	//Setup slots
    connect(serialPort, &QSerialPort::readyRead, this, &Serial::readSerialPort);
    connect(serialPort, &QSerialPort::bytesWritten, this, &Serial::writtenSerialPort);
    connect(serialPort, &QSerialPort::dataTerminalReadyChanged, this, &Serial::reportDsr);
    connect(serialPort, &QSerialPort::aboutToClose, this, &Serial::closeSerialPort);

//	connect(serialPort, SIGNAL(readyRead()), this, SLOT(readSerialPort()));
//	connect(serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(writtenSerialPort(qint64)));
//	connect(serialPort, SIGNAL(dsrChanged(bool)), this, SLOT(reportDsr(bool)));
//	connect(serialPort, SIGNAL(aboutToClose()), this, SLOT(closeSerialPort()));
	
	if (portName.startsWith("COM"))
	{
		portName.prepend("\\\\.\\");
	}

    //Every x period, request the firmware status, i.e. number of bytes read
    serialTimer = new QTimer();
	serialTimer->setTimerType(Qt::PreciseTimer);
    connect(serialTimer, &QTimer::timeout, this, &Serial::requestBytesRead);
//	connect(serialTimer, SIGNAL(timeout()), this, SLOT(requestBytesRead()));
	serialTimer->start(1000); //Request every second

    //Every x period, request the latest lux value from the sensor
	luxTimer = new QTimer();
	luxTimer->setTimerType(Qt::PreciseTimer);
    connect(luxTimer, &QTimer::timeout, this, &Serial::requestLux);
    //connect(luxTimer, SIGNAL(timeout()), this, SLOT(requestLux()));
	//luxTimer->start(2000); //Request every 2 seconds
	
    //Every 50 ms, check if there is anything to read
	readTimer = new QTimer();
	readTimer->setTimerType(Qt::PreciseTimer);
	connect(readTimer, &QTimer::timeout, this, [=]() {serialPort->waitForReadyRead(0); });
	readTimer->start(50);

    //Request firmware
	requestFirmwareVersion();

    //Check that we're ready
	emit readyForTransmit();	
}

//Destructor
Serial::~Serial()
{
	serialPort->close();
}

//Set firmware version
void Serial::setFirmwareVersion(QString firmware)
{
    pFirmwareVersion = firmware;
}

//Write an image to the serial port
void Serial::writeLEDImage(QImage led)
{
	QByteArray dataArray;

    for (int i = 0; i < led.width(); ++i)
	{
		QColor pixel = led.pixelColor(i, 0);
        BYTE b = clamp_byte(static_cast<BYTE>(pixel.blue()), 0, 250);
        BYTE g = clamp_byte(static_cast<BYTE>(pixel.green()), 0, 250);
        BYTE r = clamp_byte(static_cast<BYTE>(pixel.red()), 0, 250);

        //LED strip expects data in BGR format it seems
		dataArray.append(b);
		dataArray.append(g);
		dataArray.append(r);
	}

	//Append a 0xFF to the end to show the output
    dataArray.append(static_cast<BYTE>(ShowCommand));

    //If we're connected, write the LED to the serial port
	if (pDeviceConnected)
	{
		writeToSerialPort(dataArray);
	}	
}

//Request bytes read by Arduino
void Serial::requestBytesRead()
{
	if (pDeviceConnected)
	{
		QByteArray dataArray;
        dataArray.append(static_cast<BYTE>(ReadBytesCommand));
		writeToSerialPort(dataArray);

		if (pUpdatesMissed++ >= 3)
		{
			//If we've missed 3 updates, change status to down
			emit deviceStatusChanged(false);
			pDeviceConnected = false;
		}
	}
	else
	{
		//If currently disconnected, request firmware every update
		requestFirmwareVersion();
	}
}

//Request firmware version
void Serial::requestFirmwareVersion()
{
	QByteArray dataArray;
    dataArray.append(static_cast<BYTE>(FirmwareCommand));
	writeToSerialPort(dataArray);
}

//Request ambient light conditions
void Serial::requestLux()
{
	QByteArray dataArray;
    dataArray.append(static_cast<BYTE>(LuxCommand));
	writeToSerialPort(dataArray);
}

//Write to serial port
void Serial::writeToSerialPort(QByteArray data)
{
    long long bytesWritten = serialPort->write(data);
    Q_ASSERT(bytesWritten == data.length());
}

//Change port
void Serial::changeSerialPort(QString portName)
{
    //Check if it is actually different to the current one
    if (portName == pPortName && serialPort->isOpen())
        return;

	//Close serial port
	if (serialPort->isOpen())
		serialPort->close();

	//Change port name
	serialPort->setPortName(portName);

	//Re-open the port
	if (!serialPort->open(QIODevice::ReadWrite))
		qDebug() << "Could not open port '" << portName << "'";

    setFirmwareVersion("Not connected");
	pUpdatesMissed = 0;
	requestBytesRead();
}

//Bytes are ready to be read from serial port
void Serial::readSerialPort()
{
	if (!serialPort->bytesAvailable())
		return;

	QByteArray data = serialPort->readAll();
    pReadBuffer.append(data);

	if (!data.isEmpty())
	{
        QDataStream stream(pReadBuffer);

        while (!pReadBuffer.isEmpty())
		{
			stream.startTransaction();
			//Try to read full packet atomically
			BYTE startByte;
			BYTE type;
			BYTE length;
			BYTE payloadByte;
			QByteArray payload;

#ifdef CRC_CHECK
			quint16 crc16;
#else
			BYTE parity;
#endif

			//Read start byte from header
			stream >> startByte;

			while (startByte != START_BYTE && !stream.atEnd())
			{
				//Packet is not aligned
                pReadBuffer.remove(0, 1);
				stream >> startByte;
			}

			if (stream.atEnd())
			{
				//No more data available - wait for more
				return;
			}

			//Read type and length from header
			stream >> type >> length;

			if (length < 0 || length > 0xFF)
			{
				//Length is probably corrupt - wait for more
				return;
			}

            for (int i = 0; i < length; ++i)
			{
				stream >> payloadByte;
				payload.append(payloadByte);
			}

#ifdef CRC_CHECK
			stream >> crc16;
#else
			stream >> parity;
#endif

			if (!stream.commitTransaction())
				return;     // wait for more data so we can read the packet fully

			//If we get to this point, packet was read successfully

#ifdef PARITY_CHECK
			//Check parity
			BYTE calculatedParity8 = calculateParity8(readBuffer.constData(), SERIAL_HEADER_OVERHEAD + length);
			if (parity != calculatedParity8)
			{
				//Parity check failed - trash packet
				readBuffer.remove(0, SERIAL_PACKET_OVERHEAD + length);
				continue;
			}
#else
			//Check CRC16
            quint16 calculatedCRC16 = qChecksum(pReadBuffer.constData(), SERIAL_HEADER_OVERHEAD + length);

			if (crc16 != calculatedCRC16)
			{
				//CRC16 check failed - trash packet
                pReadBuffer.remove(0, SERIAL_PACKET_OVERHEAD + length);
				continue;
			}
#endif
			//If we get to this point, a packet has been read successfully
			switch (type)
			{
			case ReadBytesCommand:
				//It's a packet that returns how much data has been read. This is sent to the GUI
				pUpdatesMissed = 0;
				emit serialDataRead(payload);
				break;
			case FirmwareCommand:
				//It's a packet that shows the firmware. Set status and version
                setFirmwareVersion(QString::fromUtf8(payload));
				pUpdatesMissed = 0;
				if (!pDeviceConnected)
				{
					pDeviceConnected = true;
					emit deviceStatusChanged(true);
				}
				break;
			case LuxCommand:
				//It's a packet that shows the current lux value
				pUpdatesMissed = 0;
				QDataStream luxStream(payload);
				luxStream.setByteOrder(QDataStream::LittleEndian);
                luxStream >> pLux;
				emit luxValueChanged();
                break;
			}

			//We're done, let's remove the packet from the buffer
            pReadBuffer.remove(0, SERIAL_PACKET_OVERHEAD + length);
			
		}
	}
}

//Calculate the parity of 8 bytes
BYTE Serial::calculateParity8(const char* data, qint32 dataLength)
{
	//Calculate an 8 bit parity on a byte array
	if (dataLength <= 0)
		return 0;

	if (dataLength == 1)
		return data[0];

	BYTE parity = data[0];
    for (qint32 i = 1; i < dataLength; ++i)
	{
		parity ^= data[i];
	}
	return parity;
}

//===== SIGNALS FROM SERIAL PORT =====

//Written data to serial port
void Serial::writtenSerialPort(int bytes)
{
	//Amount of data has been written to serial port
	emit serialDataWritten(bytes);
}

void Serial::closeSerialPort()
{
	emit serialPortClosed();
}

void Serial::reportDsr(bool state)
{
	//DSR has changed
	emit dsrChanged(state);
}
