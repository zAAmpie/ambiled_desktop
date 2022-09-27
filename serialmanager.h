#pragma once

#include <QObject>
#include <QImage>
#include <QtSerialPort/QSerialPort>
#include <QMutex>
#include "types.h"

#define CRC_CHECK 0
#ifndef CRC_CHECK
#define PARITY_CHECK
#endif

#define START_BYTE 0xFF
#define SERIAL_HEADER_OVERHEAD 3 //Start byte + length byte + type byte
#ifdef PARITY_CHECK
#define SERIAL_CHECKSUM_OVERHEAD 1 //1 byte parity
#else
#define SERIAL_CHECKSUM_OVERHEAD 2 //2 byte CRC
#endif
#define SERIAL_PACKET_OVERHEAD SERIAL_HEADER_OVERHEAD + SERIAL_CHECKSUM_OVERHEAD //Header overhead + checksum


typedef enum {
    ShowCommand = 0xFF,
    FirmwareCommand = 0xFD,
    ReadBytesCommand = 0xFE,
    LuxCommand = 0xFC
} DeviceCommands;


class SerialManager : public QObject
{
    /* Class that handles serial communication with the AmbiLED hardware */
	Q_OBJECT

public:
    SerialManager(QString portName, QObject *parent = nullptr);
    ~SerialManager();

    //Get the serial port name
    QString getPortName() const {return pPortName;}

    //Get the firmware version
    QString getFirmwareVersion() const {return pFirmwareVersion;}

    //Get the latest lux value
    int getLux() const {return pLux;}

    //Check if the device is connected
	bool connected() { return pDeviceConnected; }

    //Set firmware version (thread-safe)
    void setFirmwareVersion(QString firmware);
	
    //Write an image to the serial port - slot
	void writeLEDImage(QImage led);

    //Change port (thread-safe)
	void changeSerialPort(QString portName);

signals:
    //Data read from serial port
    void serialDataRead(QByteArray data);

    //Amount of data written to serial port
    void serialDataWritten(int bytes);

    //Serial port can transmit more data
    void readyForTransmit();

    //DSR changed
    void dsrChanged(bool state);

    //Serial port has been closed
    void serialPortClosed();

    //Device is connected or not
    void deviceStatusChanged(bool active);

    //Lux value has changed
    void luxValueChanged();

private:
    //Write to serial port
    void writeToSerialPort(QByteArray data);

	//Bytes have been written to the serial port
    void writtenSerialPort(int bytes);
	
	//Request bytes read by Arduino
	void requestBytesRead();

	//Request firmware version
	void requestFirmwareVersion();

	//Request ambient light conditions
	void requestLux();

	//Bytes are ready to be read from serial port
	void readSerialPort();

	//DSR changed on the serial port
	void reportDsr(bool);

	//Close the serial port
	void closeSerialPort();

    //Calculate the parity of 8 bytes
    BYTE calculateParity8(const char* data, int dataLength);

    //Local class pointers
    std::unique_ptr<QSerialPort> serialPort;
    std::unique_ptr<QTimer> serialTimer;
    std::unique_ptr<QTimer> readTimer;
    std::unique_ptr<QTimer> luxTimer;

    //Local class variables
    QString pPortName;
    QString pFirmwareVersion;
    int pLux;
    QByteArray pReadBuffer;
    QMutex pMutex;

	bool pDeviceConnected;
    int pUpdatesMissed;
};
