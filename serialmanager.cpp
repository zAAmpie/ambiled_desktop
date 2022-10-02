#include "serialmanager.h"
//#include <qfile.h>
#include <qdebug.h>
#include <algorithm>
#include <qtimer.h>
#include <qdatastream.h>
#include "util.h"

//Constructor
SerialManager::SerialManager(QString portName, QObject *parent) : QObject(parent), pFirmwareVersion("Not connected"), pUpdatesMissed(0)
{
    //Create the serial port in the same thread
    pSerialPort = new QSerialPort(portName);
    pPortName = portName;
	pDeviceConnected = false;
	
	//Setup port
    pSerialPort->setBaudRate(1000000); //1MBaud
    pSerialPort->setFlowControl(QSerialPort::NoFlowControl);
    pSerialPort->setParity(QSerialPort::NoParity);
    pSerialPort->setDataBits(QSerialPort::Data8);
    pSerialPort->setStopBits(QSerialPort::OneStop);
	
    //Initialize port
	changeSerialPort(portName);

	//Setup slots
    connect(pSerialPort, &QSerialPort::readyRead, this, &SerialManager::serialPortReadyRead);
    connect(pSerialPort, &QSerialPort::bytesWritten, this, &SerialManager::serialDataWritten); //signal to signal
    connect(pSerialPort, &QSerialPort::dataTerminalReadyChanged, this, &SerialManager::dsrChanged); //signal to signal
    connect(pSerialPort, &QSerialPort::aboutToClose, this, &SerialManager::serialPortClosed); //signal to signal
	
	if (portName.startsWith("COM"))
	{
		portName.prepend("\\\\.\\");
	}

    //Every x period, request the firmware status, i.e. number of bytes read
    pSerialTimer = new QTimer();
    pSerialTimer->setTimerType(Qt::PreciseTimer);
    connect(pSerialTimer, &QTimer::timeout, this, &SerialManager::requestBytesRead);
    pSerialTimer->start(1000); //Request every second

    //Every x period, request the latest lux value from the sensor
    pLuxTimer = new QTimer();
    pLuxTimer->setTimerType(Qt::PreciseTimer);
    connect(pLuxTimer, &QTimer::timeout, this, &SerialManager::requestLux);
	//luxTimer->start(2000); //Request every 2 seconds
	
    //Every 50 ms, check if there is anything to read
    pReadTimer = new QTimer();
    pReadTimer->setTimerType(Qt::PreciseTimer);
    connect(pReadTimer, &QTimer::timeout, this, [=]() {pSerialPort->waitForReadyRead(0); });
    pReadTimer->start(50);

    //Request firmware
	requestFirmwareVersion();

    //Check that we're ready
	emit readyForTransmit();	
}

//Destructor
SerialManager::~SerialManager()
{
    pSerialPort->close();
}

//Set firmware version
void SerialManager::setFirmwareVersion(QString firmware)
{
    pMutex.lock();
    pFirmwareVersion = firmware;
    pMutex.unlock();
}

//Write an image to the serial port
void SerialManager::writeLEDImage(QImage led)
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
void SerialManager::requestBytesRead()
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
void SerialManager::requestFirmwareVersion()
{
	QByteArray dataArray;
    dataArray.append(static_cast<BYTE>(FirmwareCommand));
	writeToSerialPort(dataArray);
}

//Request ambient light conditions
void SerialManager::requestLux()
{
	QByteArray dataArray;
    dataArray.append(static_cast<BYTE>(LuxCommand));
	writeToSerialPort(dataArray);
}

//Write to serial port
void SerialManager::writeToSerialPort(QByteArray data)
{
    long long bytesWritten = pSerialPort->write(data);
    Q_ASSERT(bytesWritten == data.length());
}

//Change port
void SerialManager::changeSerialPort(QString portName)
{
    //Check if it is actually different to the current one
    if (portName == pPortName && pSerialPort->isOpen())
        return;

    pMutex.lock();

	//Close serial port
    if (pSerialPort->isOpen())
        pSerialPort->close();

	//Change port name
    pSerialPort->setPortName(portName);

	//Re-open the port
    if (!pSerialPort->open(QIODevice::ReadWrite))
		qDebug() << "Could not open port '" << portName << "'";

    pMutex.unlock();

    setFirmwareVersion("Not connected");
	pUpdatesMissed = 0;
	requestBytesRead();
}

//Bytes are ready to be read from serial port
void SerialManager::serialPortReadyRead()
{
    if (!pSerialPort->bytesAvailable())
		return;

    QByteArray data = pSerialPort->readAll();
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
            quint16 calculatedCRC16 = qChecksum(QByteArrayView(pReadBuffer.constData(), SERIAL_HEADER_OVERHEAD + length));

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
BYTE SerialManager::calculateParity8(const char* data, qint32 dataLength)
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
