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
#include "perfstats.h"
#include "testpattern.h"

Q_DECLARE_METATYPE(LEDPositions)
Q_DECLARE_METATYPE(ImageStrips)

class QListWidgetItem;
class QComboBox;
class QCheckBox;
class QButtonGroup;
class QStackedWidget;
class ParameterForm;

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
        ProcessingSetting,
        ColourCorrectionPresetSetting,
        ColourCorrectionRedSetting,
        ColourCorrectionGreenSetting,
        ColourCorrectionBlueSetting,
        LEDPositionsTopSetting,
        LEDPositionsLeftSetting,
        LEDPositionsRightSetting,
        LEDPositionsBottomLeftSetting,
        LEDPositionsBottomGapSetting,
        LEDPositionsBottomRightSetting,
        WaylandRestoreTokenSetting
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
    //Screen Manager's Wayland backend issued a new/rotated portal restore_token
    void screenManagerWaylandRestoreTokenUpdated(QString token);

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
    void uiScreenCaptureToggled(bool enabled);
    //Test tab: where the test pattern goes (off / direct into processing) changed
    void uiTestOutputChanged();
    //Test tab: the chosen pattern or any of its parameters changed
    void uiTestPatternChanged();
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
    //A process's checkbox was toggled in the processing list (enable/disable)
    void uiProcessListItemChanged(QListWidgetItem *item);
    //The current item in the processing list changed (switch which config page is shown)
    void uiProcessListCurrentItemChanged(QListWidgetItem *current);
    //Any config widget on the currently shown process config page changed value
    void uiProcessConfigValueChanged();
    //Scale's interpolation mode combo changed - not a process parameter, but a choice of
    //which ProcessType is the active one (see createProcessConfigPage)
    void uiScaleModeChanged(int index);
    //Clicked on system tray
    void uiTrayClicked(QSystemTrayIcon::ActivationReason reason);
    //Quit was selected from the tray menu
    void uiQuitTriggered();

    //===== CLASS FUNCTIONS =====
    //Display image on label
    void displayStrips(ImageStrips outLines);
    //Display full screen on label
    void displayFullScreen(QImage image);
    //Update the UI LED image at the bottom
    void displayLEDStrip(QImage image);
    //Live previews only redraw at PreviewFrameRate - painting them every frame cost far more than
    //the capture pipeline itself. True (and the deadline advanced) if a preview redraw is due now.
    bool previewDue(qint64 &nextDueMs);
    static constexpr int PreviewFrameRate = 20;
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

    //===== GENERIC PROCESS CONFIGURATION UI =====
    //Builds the processing list rows and their config pages; needs pProcessManager to already
    //exist, so it's called from the constructor rather than from setupGUI()
    void setupProcessingList();
    //Builds a config page (a ParameterForm over the type's ParameterSpecs) for the given process type,
    //adds it to ui.processingConfigStack, and records it for later lookup
    QWidget *createProcessConfigPage(ImageProcess::ProcessType type);
    //Fills in the given process type's config page widgets from ProcessManager's current values
    void populateProcessConfigPage(ImageProcess::ProcessType type);
    QMap<ImageProcess::ProcessType, QWidget *> pProcessConfigPages;
    //Scale's extra interpolation-mode combo (see uiScaleModeChanged)
    QComboBox *pScaleModeComboBox;
    //Which of SampleScale/LinearScale/CubicScale the single "Scale" list row currently represents.
    //UI-local bookkeeping only - ProcessManager has no concept of "the active scale type".
    ImageProcess::ProcessType pActiveScaleType;

    //===== TEST TAB =====
    //Where a test pattern goes - nothing in the Test tab is persisted, so tests never resume at startup
    enum TestOutput {TestOutputOff, TestOutputDirect};
    //Builds the Test tab (output choice, pattern choice + a ParameterForm per pattern, verbose serial log)
    QWidget *createTestTab();
    //The pattern currently configured in the Test tab
    TestPattern currentTestPattern() const;
    QButtonGroup *pTestOutputGroup;
    QComboBox *pTestPatternComboBox;
    QStackedWidget *pTestPatternPages;
    QList<ParameterForm *> pTestPatternForms; //Indexed like pTestPatternComboBox/pTestPatternPages
    QCheckBox *pVerboseSerialCheckBox;

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
    QElapsedTimer pPreviewClock;
    qint64 pNextScreenPreviewMs = 0;
    qint64 pNextStripsPreviewMs = 0;

    //GUI-thread perf stats (see perfstats.h)
    PerfStats pGuiFrameStats{QStringLiteral("gui.frame")};
    PerfStats pGuiStripsStats{QStringLiteral("gui.strips")};
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
    bool pCaptureIdleMode;
};
