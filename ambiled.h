#pragma once

//<div>Icons made by <a href="https://www.flaticon.com/authors/xnimrodx" title="xnimrodx">xnimrodx</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QElapsedTimer>
#include <QSerialPortInfo>
#include <QQueue>
#include <QSettings>
#include "serialmanager.h"
#include "ui_ambiled.h"
#include "execthread.h"
#include "version.h"
#include "screenmanager.h"
#include "processmanager.h"
#include "leds.h"

Q_DECLARE_METATYPE(LEDPositions)
Q_DECLARE_METATYPE(ImageStrips)

class AmbiLED : public QMainWindow
{
	Q_OBJECT

public:
    AmbiLED(QWidget *parent = nullptr);
	~AmbiLED();

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
    //Re-implemented functions to intercept closing/minimising the UI
	void closeEvent(QCloseEvent *event);
	void hideEvent(QHideEvent* event);

signals:
    //===== SCREEN MANAGER SIGNALS FOR SLOTS =====
    //Nothing

    //===== LEDS SLOTS FOR SIGNALS =====
    //Nothing

    //===== PROCESS MANAGER SIGNALS FOR SLOTS =====
    //Process the input screen and provide an LED output
    void processManagerStartProcess(QImage screen, StripSizes sizes);
    //TODO: Add full configuration of process manager

    //===== SERIAL MANAGER SIGNALS FOR SLOTS =====
    //Write an image to the serial port
    void serialManagerWriteLEDImage(QImage led);

private:
    //===== SCREEN MANAGER SLOTS FOR SIGNALS =====
    //Final processed image is ready
    void screenManagerReadyFrame(QImage screenImage);
    //Screen Manager signalled failure
    void screenManagerFailed(QString message);

    //===== LEDS SLOTS FOR SIGNALS =====
    //No signals

    //===== PROCESS MANAGER SLOTS FOR SIGNALS =====
    //Processing has been completed
    void processManagerReadyProcess(ImageStrips strips);
    //Processing failure
    void processManagerFailed(QString message);

    //===== SERIAL MANAGER SLOTS FOR SIGNALS =====
    //Data read from serial port
    void serialManagerSerialDataRead(QByteArray data);
    //Amount of data written to serial port
    void serialManagerSerialDataWritten(int bytes);
    //Serial port can transmit more data
    void serialManagerReadyForTransmit();
    //Device is connected or not
    void serialManagerDeviceStatusChanged(bool active);
    //Lux value has changed
    void serialManagerLuxValueChanged();
    //Serial port failure (error message)
    void serialManagerFailed(QString message);

    //===== UI SLOTS FOR SIGNALS =====
    //Toggle screen capture
    void uiScreenCaptureToggled();
    //Debug mode toggled
    void uiDebugModeToggled();
    //Visualise fullscreen mode toggled
    void uiFullscreenModeToggled();
    //The refresh rate has been changed
    void uiRefreshRateChanged(int index);
    //Slider has been moved to change brightness
    void uiBrightnessChanged(int value);
    //Capture mode has been changed
    void uiCaptureModeChanged(int index);
    //LED positions have been changed
    void uiLedPositionsChanged();
    //Serial port selection has changed
    void uiSerialPortChanged(int index);
    //Colour correction sliders have been changed
    void uiColourTempSlidersChanged();
    //Colour correction preset has been changed
    void uiColourTempComboChanged(int index);
    //Averaging method preset has been changed
    void uiAverageMethodComboChanged(int index);
    //Selection was changed in the processing list
    void uiProcessListSelectionChanged();
    //Clicked on system tray
    void uiTrayClicked(QSystemTrayIcon::ActivationReason reason);

    //===== CLASS FUNCTIONS =====
    //Display image on label
    void displayStrips(ImageStrips outLines);
    //Display full screen on label
    void displayFullScreen(QImage image);
    //Update the UI LED image at the bottom
    void displayLEDStrip(QImage image);
    //Update UI elements that require it
    void updateUIElements();
    //Show popup with device firmware and information
    void showDeviceFirmware();
    //Setting related
    void readSettings();
    void writeSettings();
    //Set idle mode
    void setIdleMode(bool state);
    void enableIdleMode() {setIdleMode(true);}
    void disableIdleMode() {setIdleMode(false);}

    //===== UI SETUP FUNCTIONS =====
    void createActions();
    void createTrayIcon();
    void createPixmaps();
    void createSettingsMap();
    void setupGUI();

    //Pointers to bigger objects
    SerialManager *pSerialManager;
    ScreenManager *pScreenManager;
    ProcessManager *pProcessManager;
    std::unique_ptr<LEDS> pLeds;
    ExecThread *pCaptureThread;
    ExecThread *pProcessThread;

    Ui::AmbiLEDClass ui;

    //UI variables
    QAction *pQuitAction;
    QAction *pConfigureAction;
    QAction *pSuspendAction;

    QPixmap pConnectedPixmap;
    QPixmap pDisconnectedPixmap;

    QSystemTrayIcon *pTray;
    QMenu *pTrayMenu;

    //Timers
    QElapsedTimer *pElapsedFrameTimer;
    QElapsedTimer *pElapsedSerialTimer;
    QTimer *pUiUpdateTimer;

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
    bool pCaptureEnabled;
    bool pFullscreenMode;
    bool pDebugMode;
    bool pCaptureIdleMode;
};
