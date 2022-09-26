#pragma once

//Attribute icon
//<div>Icons made by <a href="https://www.flaticon.com/authors/xnimrodx" title="xnimrodx">xnimrodx</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QElapsedTimer>
#include <QSerialPortInfo>
#include "util.h"
#include "serial.h"
#include "ui_ambiled.h"
#include "execthread.h"
#include "version.h"
#include <QQueue>
#include <QSettings>
#include "screenmanager.h"
#include "leds.h"

Q_DECLARE_METATYPE(LEDPositions)
Q_DECLARE_METATYPE(ImageStripsInput)

class QMainWindow;

class AmbiLED : public QMainWindow
{
	Q_OBJECT

public:
    AmbiLED(QWidget *parent = nullptr);
	~AmbiLED();
    enum ArrowDirection { LeftArrow, RightArrow, UpArrow, DownArrow };
    enum AmbiLEDSettings {
        SerialPortSetting,
        RefreshRateSetting,
        BrightnessSetting,
        VisualiseSetting,
        CaptureMethodSetting,
        AveragingMethodSetting,
        ColourCorrectionPresetSetting,
        ColourCorrectionRedSetting,
        ColourCorrectionGreenSetting,
        ColourCorrectionBlueSetting,
        LEDPositionsTopSetting,
        LEDPositionsLeftSetting,
        LEDPositionsRightSetting,
        LEDPositionsBottomLeftSetting,
        LEDPositionsBottomGapSetting,
        LEDPositionsBottomRightSetting
    };
    Q_ENUM(AmbiLEDSettings)


protected:
	void closeEvent(QCloseEvent *event);

	void hideEvent(QHideEvent* event);

private slots:
	//Capture the screen for decoding
	void captureScreen();

	//Display image on label
    void displayStrips(ImageStripsInput outLines);

	//Display full screen on label
    void displayFullScreen(QImage image);

	//Notification from screen capture that it has failed
	void captureFailed();

	//Send LED data
	void sendLEDData(QImage led);

	//Slot to get data from serial port
	void serialPortDataRead(QByteArray data);

	//Slot to set serial port as ready
	void serialPortReady();

	//Slot called when lux should be updated
	void luxValueChanged();

	//Toggle screen capture
	void toggleCapture();

	//The refresh rate has been changed
    void refreshRateChanged(int index);

	//Slider has been moved to change brightness
    void brightnessChanged(int value);

	//Serial port has been changed
    void serialPortChanged(int);

	//Capture mode has been changed
    void captureModeChanged(int index);

	//LED positions have been changed
	void ledPositionsChanged();

	//Colour correction sliders have been changed
    void colourTemperatureChanged();

	//Colour correction preset has been changed
    void colourTemperatureComboChanged(int index);

	//Averaging method preset has been changed
    void averageMethodComboChanged(int index);

	//Serial port wrote some data
    void serialPortDataWritten(int bytes);

	//Serial device status changed
	void serialDeviceStatusChanged(bool status);

    //Debug mode toggled
    void toggleDebugMode();

    //Visualise fullscreen mode toggled
    void toggleFullscreenMode();

	//Clicked on system tray
	void trayClicked(QSystemTrayIcon::ActivationReason reason);

	//Update UI elements that require it
	void updateUIElements();

	//Show popup with device firmware and information
	void showDeviceFirmware();

	//Setting related
	void readSettings();
	void writeSettings();

signals:
    void updateScreenCapture(const LEDPositions &positions);

	//Write an image to the serial port
	void writeLEDImage(QImage led);

	//Update serial port name
	void updateSerialPort(QString portName);
private:
	void createActions();
	void createTrayIcon();
	void createPixmaps();
    void createSettingsMap();
    void setupGUI();

    QPixmap &drawArrow(QPixmap& input, ArrowDirection direction, Qt::GlobalColor colour = Qt::yellow, int offset = 0, qreal height = -1.0, qreal width = -1.0, QString text = "");
	QPixmap& drawAverageBars(QPixmap& input, Qt::GlobalColor colour);
    //void transformBrightness(QImage &led, qreal brightness = 1.0);

    //Pointers to bigger objects
    Serial *serialPort;

    ScreenManager *capture;
    LEDS *leds;
    ExecThread *pCaptureThread;

    Ui::AmbiLEDClass ui;



    //UI variables
    QAction *quitAction;
    QAction *configureAction;
    QAction *suspendAction;

	QPixmap connectedPixmap;
	QPixmap disconnectedPixmap;

    QSystemTrayIcon *tray;
    QMenu *trayMenu;

    //Timers
	QElapsedTimer *elapsedTimer;
	QElapsedTimer *elapsedSerialTimer;
    QTimer *pScreenUpdateTimer;
	QTimer* uiUpdateTimer;

    //Local variables
    QQueue<qint64> pFrameAverageQueue;
    QQueue<qint64> pSerialAverageQueue;

    QMap<AmbiLEDSettings, QString> pSettingsMap;
    QSettings *pSettings;

    int pReadCounter;
    int pFrameCounter;
    long long pSerialBytesWritten;

    //Current states
    bool pNotificationShown;
    bool pSerialPortReady;
    bool pCaptureScreen;
    bool pDebugMode;
    bool pCaptureIdleMode;
};
