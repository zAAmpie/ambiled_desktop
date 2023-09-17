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
    SerialManager(QObject* parent = nullptr); //Unspecified port constructor
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
    //Change port (thread-safe)
    void changeSerialPort(QString portName);
    //Reconnect (thread-safe)
    void reconnect();
	
    //Write an image to the serial port - slot
	void writeLEDImage(QImage led);
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
    //Failure
    void failed(QString message);

private:
    void initialise(QString portName);

    //===== SLOTS FOR SERIALPORT =====
    //Bytes are ready to be read from serial port
    void serialPortReadyRead();

    //Write to serial port
    void writeToSerialPort(QByteArray data);
	//Request bytes read by Arduino
	void requestBytesRead();
	//Request firmware version
	void requestFirmwareVersion();
	//Request ambient light conditions
	void requestLux();

    //Calculate the parity of 8 bytes
    BYTE calculateParity8(const char* data, int dataLength);

    //Local class pointers
    QSerialPort *pSerialPort;
    QTimer *pSerialTimer;
    QTimer *pReadTimer;
    QTimer *pLuxTimer;

    //Local class variables
    QString pPortName;
    QString pFirmwareVersion;
    int pLux;
    QByteArray pReadBuffer;
    QMutex pMutex;

	bool pDeviceConnected;
    int pUpdatesMissed;
    bool pInitialised;
};
