#include <QtWidgets>
#include "ambiled.h"
#include "pixmaputils.h"

//Constructor
AmbiLED::AmbiLED(QWidget *parent)
    : QMainWindow(parent),     //Set up local variables
      pScaleModeComboBox(nullptr),
      pActiveScaleType(ImageProcess::LinearScale),
      pReadCounter(0),
      pFrameCounter(0),
      pSerialBytesWritten(0),
      pNotificationShown(false),
      pSerialPortReady(false),
      pCaptureEnabled(true),
      pCaptureIdleMode(false)
{
    //Register metatypes to use in signal/slot system
    qRegisterMetaType<ImageStrips>("ImageStripsInput");
	qRegisterMetaType<LEDPositions>("LEDPositions");

    //Create map of enum
    createSettingsMap();
	//Create new settings object
	pSettings = new QSettings("AmbiLED", "AmbiLED");

    //Read this ahead of readSettings() (which runs at the end of this constructor): the Wayland
    //backend starts negotiating with the portal as soon as it's constructed below, so the
    //restore_token needs to be ready before that, not after.
    QString waylandRestoreToken = pSettings->value(pSettingsMap.value(WaylandRestoreTokenSetting)).toString();

    //Set up initial UI
    setupGUI();

    //Set up serial port options
    pSerialManager = new SerialManager();
    serialManagerDeviceStatusChanged(false);
	foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts())
	{
		ui.comPortComboBox->addItem(info.portName().isEmpty() ? info.description() : info.portName());
	}
    ui.comPortComboBox->setCurrentIndex(0);
    
	//Create serial port object
    connect(this, &AmbiLED::serialManagerWriteLEDImage, pSerialManager, &SerialManager::writeLEDImage);
    connect(pSerialManager, &SerialManager::readyForTransmit, this, &AmbiLED::serialManagerReadyForTransmit);
    connect(pSerialManager, &SerialManager::serialDataRead, this, &AmbiLED::serialManagerSerialDataRead);
    connect(pSerialManager, &SerialManager::serialDataWritten, this, &AmbiLED::serialManagerSerialDataWritten);
    connect(pSerialManager, &SerialManager::deviceStatusChanged, this, &AmbiLED::serialManagerDeviceStatusChanged);
    connect(pSerialManager, &SerialManager::luxValueChanged, this, &AmbiLED::serialManagerLuxValueChanged);

	pSerialPortReady = true;

    //Create LED object
    pLeds = std::unique_ptr<LEDS>(new LEDS());

    //Create screen manager object
    pScreenManager = new ScreenManager(ui.refreshRateComboBox->currentData().toInt(), waylandRestoreToken);
    connect(pScreenManager, &ScreenManager::readyFrame, this, &AmbiLED::screenManagerReadyFrame);
    connect(pScreenManager, &ScreenManager::failed, this, &AmbiLED::screenManagerFailed);
    connect(pScreenManager, &ScreenManager::waylandRestoreTokenUpdated, this, &AmbiLED::screenManagerWaylandRestoreTokenUpdated);
    uiCaptureModeChanged(ui.captureComboBox->currentIndex());
    uiFullscreenModeToggled();

    //Move capturing of the screen to another thread. Will need to see if that's possible with X11
    pCaptureThread = new ExecThread();
    pCaptureThread->setObjectName(QStringLiteral("ambiled-capture")); //Becomes the OS thread name - visible in perf/pidstat/top
    pScreenManager->moveToThread(pCaptureThread);
    pCaptureThread->start();

    //Create process manager object
    pProcessManager = new ProcessManager();
    connect(pProcessManager, &ProcessManager::readyProcess, this, &AmbiLED::processManagerReadyProcess);
    connect(pProcessManager, &ProcessManager::failed, this, &AmbiLED::processManagerFailed);
    //Direct: startProcess() is thread-safe and does its own latest-frame-wins handoff to the process thread
    connect(this, &AmbiLED::processManagerStartProcess, pProcessManager, &ProcessManager::startProcess, Qt::DirectConnection);
    uiAverageMethodComboChanged(ui.averageMethodComboBox->currentIndex());
    pProcessManager->setEnabledProcess(ImageProcess::BlackBarRemoval, true); //Set black bar removal by default

    //Build the processing list UI now that pProcessManager exists to query defaults from -
    //readSettings() (end of constructor) will override enabled/parameters from saved settings
    setupProcessingList();

    //Try moving process manager to another thread
    pProcessThread = new ExecThread();
    pProcessThread->setObjectName(QStringLiteral("ambiled-process"));
    pProcessManager->moveToThread(pProcessThread);
    pProcessThread->start();

	//Create elapsed timer for FPS
    pElapsedFrameTimer = new QElapsedTimer();
    pElapsedFrameTimer->start();

    //Create elapsed timer for Serial writes
    pElapsedSerialTimer = new QElapsedTimer();
    pElapsedSerialTimer->start();

    //Notify the relevant objects of the LED positions
    uiLedPositionsChanged();

    //Update GUI with relevant statistics
    pUiUpdateTimer = new QTimer();
    pUiUpdateTimer->setTimerType(Qt::PreciseTimer);
    connect(pUiUpdateTimer, &QTimer::timeout, this, &AmbiLED::updateUIElements);
    pUiUpdateTimer->start(200); //Update at a rate of 5Hz

    //Read and populate previous settings
	readSettings();
}

//Destructor
AmbiLED::~AmbiLED()
{
    pCaptureThread->quit();
    if (pCaptureThread->wait(2000))
        delete pCaptureThread;
    else
    {
        pCaptureThread->terminate();
        delete pCaptureThread;
    }
	
	//Free frame buffer data
    pScreenManager->deleteLater();
    pSerialManager->deleteLater();
}

//Set up UI
void AmbiLED::setupGUI()
{
    //Set up GUI
    ui.setupUi(this);

    //Create pixmaps
    createPixmaps();

    //Set up default style of strip and fullscreen labels
    QString placeHolderStyle = tr("QLabel { background-color : black; }");
    ui.screenLabel->setStyleSheet(placeHolderStyle);
    ui.TLlabel->setStyleSheet(placeHolderStyle);
    ui.TRlabel->setStyleSheet(placeHolderStyle);
    ui.BLlabel->setStyleSheet(placeHolderStyle);
    ui.BRlabel->setStyleSheet(placeHolderStyle);

    //Set up COM port
    connect(ui.comPortComboBox, &QComboBox::currentIndexChanged, this, &AmbiLED::uiSerialPortChanged);

    //Enumerate refresh rate options
    ui.refreshRateComboBox->addItem(tr("Maximum"), 100);
    ui.refreshRateComboBox->addItem(tr("60 Hz"), 60);
    ui.refreshRateComboBox->addItem(tr("50 Hz"), 50);
    ui.refreshRateComboBox->addItem(tr("30 Hz"), 30);
    ui.refreshRateComboBox->addItem(tr("25 Hz"), 25);
    ui.refreshRateComboBox->addItem(tr("24 Hz"), 24);
    ui.refreshRateComboBox->addItem(tr("20 Hz"), 20);
    ui.refreshRateComboBox->addItem(tr("15 Hz"), 15);
    ui.refreshRateComboBox->addItem(tr("10 Hz"), 10);
    ui.refreshRateComboBox->addItem(tr("5 Hz"), 5);
    ui.refreshRateComboBox->addItem(tr("1 Hz"), 1);
    ui.refreshRateComboBox->setCurrentIndex(3);
    connect(ui.refreshRateComboBox, &QComboBox::currentIndexChanged, this, &AmbiLED::uiRefreshRateChanged);

    //Set up capture mode
#ifdef Q_OS_WIN
    ui.captureComboBox->addItem(tr("DXGI"), ScreenCapture::DXGIMode);
    ui.captureComboBox->addItem(tr("DirectX 9"), ScreenCapture::DirectX9Mode);
    ui.captureComboBox->addItem(tr("DirectX 11"), ScreenCapture::DirectX11Mode);
    ui.captureComboBox->addItem(tr("GDI"), ScreenCapture::GDIMode);
#endif
#ifdef Q_OS_UNIX
    //Inserted before X11 so index 0 matches whichever backend ScreenManager's constructor
    //auto-selects (see ScreenManager::isWaylandSession()) - keep this ordering in sync with that.
    if (QGuiApplication::platformName() == QLatin1String("wayland"))
        ui.captureComboBox->addItem(tr("Wayland (PipeWire)"), ScreenCapture::WaylandMode);
    ui.captureComboBox->addItem(tr("X11"), ScreenCapture::X11Mode);
#endif
#ifdef Q_OS_WIN
    ui.captureComboBox->setCurrentIndex(1);
#else
    ui.captureComboBox->setCurrentIndex(0);
#endif
    connect(ui.captureComboBox, &QComboBox::currentIndexChanged, this, &AmbiLED::uiCaptureModeChanged);

    //Set up brightness slider
    connect(ui.brightnessSlider, &QSlider::valueChanged, this, &AmbiLED::uiBrightnessChanged);
    connect(ui.brightnessSlider, &QSlider::valueChanged, this, [=]() {ui.brightnessValueLabel->setText(tr("%1%").arg(ui.brightnessSlider->value())); });
    ui.brightnessValueLabel->setText(tr("%1%").arg(ui.brightnessSlider->value()));

    //Set up capture connection
    connect(ui.captureCheckBox, &QCheckBox::toggled, this, &AmbiLED::uiScreenCaptureToggled);

    //Set up debug connection
    connect(ui.debugCheckBox, &QCheckBox::toggled, this, &AmbiLED::uiDebugModeToggled);

    //Set up visualise connection
    connect(ui.visualiseCheckBox, &QCheckBox::toggled, this, &AmbiLED::uiFullscreenModeToggled);

    //Set up colour correction selection
    ui.colourTemperatureComboBox->addItem(tr("Candle"), LEDS::Candle);
    ui.colourTemperatureComboBox->addItem(tr("Tungsten 40W"), LEDS::Tungsten40W);
    ui.colourTemperatureComboBox->addItem(tr("Tungsten 100W"), LEDS::Tungsten100W);
    ui.colourTemperatureComboBox->addItem(tr("Halogen"), LEDS::Halogen);
    ui.colourTemperatureComboBox->addItem(tr("Carbon Arc"), LEDS::CarbonArc);
    ui.colourTemperatureComboBox->addItem(tr("High Noon Sun"), LEDS::HighNoonSun);
    ui.colourTemperatureComboBox->addItem(tr("Direct Sunlight"), LEDS::DirectSunlight);
    ui.colourTemperatureComboBox->addItem(tr("Overcast Sky"), LEDS::OvercastSky);
    ui.colourTemperatureComboBox->addItem(tr("Clear Blue Sky"), LEDS::ClearBlueSky);
    ui.colourTemperatureComboBox->addItem(tr("Warm Fluorescent"), LEDS::WarmFluorescent);
    ui.colourTemperatureComboBox->addItem(tr("Standard Fluorescent"), LEDS::StandardFluorescent);
    ui.colourTemperatureComboBox->addItem(tr("Cool White Fluorescent"), LEDS::CoolWhiteFluorescent);
    ui.colourTemperatureComboBox->addItem(tr("Full Spectrum Fluorescent"), LEDS::FullSpectrumFluorescent);
    ui.colourTemperatureComboBox->addItem(tr("Grow Light Fluorescent"), LEDS::GrowLightFluorescent);
    ui.colourTemperatureComboBox->addItem(tr("Black Light Fluorescent"), LEDS::BlackLightFluorescent);
    ui.colourTemperatureComboBox->addItem(tr("Mercury Vapor"), LEDS::MercuryVapor);
    ui.colourTemperatureComboBox->addItem(tr("Sodium Vapor"), LEDS::SodiumVapor);
    ui.colourTemperatureComboBox->addItem(tr("Metal Halide"), LEDS::MetalHalide);
    ui.colourTemperatureComboBox->addItem(tr("High Pressure Sodium"), LEDS::HighPressureSodium);
    ui.colourTemperatureComboBox->addItem(tr("Uncorrected Temperature"), LEDS::UncorrectedTemperature);
    ui.colourTemperatureComboBox->addItem(tr("Custom"), 0);
    ui.colourTemperatureComboBox->setCurrentIndex(19);
    connect(ui.colourTemperatureComboBox, &QComboBox::currentIndexChanged, this, &AmbiLED::uiColourTempComboChanged);

    //Set up custom colour sliders
    connect(ui.redColourSlider, &QSlider::valueChanged, this, [=]() {ui.redColourLabel->setText(QString::number(ui.redColourSlider->value())); });
    connect(ui.greenColourSlider, &QSlider::valueChanged, this, [=]() {ui.greenColourLabel->setText(QString::number(ui.greenColourSlider->value())); });
    connect(ui.blueColourSlider, &QSlider::valueChanged, this, [=]() {ui.blueColourLabel->setText(QString::number(ui.blueColourSlider->value())); });

    connect(ui.redColourSlider, &QSlider::valueChanged, this, &AmbiLED::uiColourTempSlidersChanged);
    connect(ui.greenColourSlider, &QSlider::valueChanged, this, &AmbiLED::uiColourTempSlidersChanged);
    connect(ui.blueColourSlider, &QSlider::valueChanged, this, &AmbiLED::uiColourTempSlidersChanged);

    //Set up averaging mode
    ui.averageMethodComboBox->addItem(tr("Simple Averaging (Basic/Fastest)"), ImageTransform::SimpleTransform);
    ui.averageMethodComboBox->addItem(tr("Strip Averaging (Good/Fast)"), ImageTransform::StripTransform);
    ui.averageMethodComboBox->addItem(tr("Block/Strip Averaging (Nice/Slow)"), ImageTransform::BlockStripTransform);
    ui.averageMethodComboBox->addItem(tr("Block Averaging (Great/Slowest)"), ImageTransform::BlockTransform);
    connect(ui.averageMethodComboBox, &QComboBox::currentIndexChanged, this, &AmbiLED::uiAverageMethodComboChanged);

    //Set up LED positions
    connect(ui.bottomLeftLEDCount, &QSpinBox::valueChanged, this, &AmbiLED::uiLedPositionsChanged);
    connect(ui.bottomRightLEDCount, &QSpinBox::valueChanged, this, &AmbiLED::uiLedPositionsChanged);
    connect(ui.gapLEDCount, &QSpinBox::valueChanged, this, &AmbiLED::uiLedPositionsChanged);
    connect(ui.leftLEDCount, &QSpinBox::valueChanged, this, &AmbiLED::uiLedPositionsChanged);
    connect(ui.rightLEDCount, &QSpinBox::valueChanged, this, &AmbiLED::uiLedPositionsChanged);
    connect(ui.topLEDCount, &QSpinBox::valueChanged, this, &AmbiLED::uiLedPositionsChanged);

    //Set up device firmware label connetion
    connect(ui.deviceStatusColourLabel, &QPushButton::clicked, this, &AmbiLED::showDeviceFirmware);

    //Set up save/discard button
    connect(ui.saveConfigButton, &QPushButton::clicked, this, &AmbiLED::writeSettings);
    connect(ui.discardConfigButton, &QPushButton::clicked, this, &AmbiLED::readSettings);

    //Create tray icon
    createActions();
    createTrayIcon();
    pTray->show();

    //Set up icon and title
    setWindowIcon(QIcon(":/AmbiLED/monitor_icon"));
    setWindowTitle(tr("AmbiLED v%1").arg(VERSION_STR));
    //this->resize(800, 600);
}

//Enumerate over enum and return an Enum - String pair
void AmbiLED::createSettingsMap()
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<AmbiLEDSettings>();
    for (int i = 0; i < metaEnum.keyCount(); ++i)
    {
        pSettingsMap.insert(static_cast<AmbiLEDSettings>(metaEnum.value(i)), QString(metaEnum.key(i)));
    }
}

//Read the previous settings
void AmbiLED::readSettings()
{
    if (pSettings->contains(pSettingsMap.value(SerialPortSetting)))
	{
        QString portName = pSettings->value(pSettingsMap.value(SerialPortSetting)).toString();
		ui.comPortComboBox->setCurrentText(portName);
        pSerialManager->changeSerialPort(portName);
	}
    if (pSettings->contains(pSettingsMap.value(RefreshRateSetting)))
        ui.refreshRateComboBox->setCurrentText(pSettings->value(pSettingsMap.value(RefreshRateSetting)).toString());
    if (pSettings->contains(pSettingsMap.value(BrightnessSetting)))
        ui.brightnessSlider->setValue(pSettings->value(pSettingsMap.value(BrightnessSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(VisualiseSetting)))
        ui.visualiseCheckBox->setChecked(pSettings->value(pSettingsMap.value(VisualiseSetting)).toBool());
    if (pSettings->contains(pSettingsMap.value(CaptureMethodSetting)))
        ui.captureComboBox->setCurrentText(pSettings->value(pSettingsMap.value(CaptureMethodSetting)).toString());
    if (pSettings->contains(pSettingsMap.value(AveragingMethodSetting)))
        ui.averageMethodComboBox->setCurrentText(pSettings->value(pSettingsMap.value(AveragingMethodSetting)).toString());

    if (pSettings->contains(pSettingsMap.value(ColourCorrectionPresetSetting)))
        ui.colourTemperatureComboBox->setCurrentText(pSettings->value(pSettingsMap.value(ColourCorrectionPresetSetting)).toString());
    if (pSettings->contains(pSettingsMap.value(ColourCorrectionRedSetting)))
        ui.redColourSlider->setValue(pSettings->value(pSettingsMap.value(ColourCorrectionRedSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(ColourCorrectionGreenSetting)))
        ui.greenColourSlider->setValue(pSettings->value(pSettingsMap.value(ColourCorrectionGreenSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(ColourCorrectionBlueSetting)))
        ui.blueColourSlider->setValue(pSettings->value(pSettingsMap.value(ColourCorrectionBlueSetting)).toInt());

    if (pSettings->contains(pSettingsMap.value(LEDPositionsTopSetting)))
        ui.topLEDCount->setValue(pSettings->value(pSettingsMap.value(LEDPositionsTopSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(LEDPositionsLeftSetting)))
        ui.leftLEDCount->setValue(pSettings->value(pSettingsMap.value(LEDPositionsLeftSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(LEDPositionsRightSetting)))
        ui.rightLEDCount->setValue(pSettings->value(pSettingsMap.value(LEDPositionsRightSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(LEDPositionsBottomLeftSetting)))
        ui.bottomLeftLEDCount->setValue(pSettings->value(pSettingsMap.value(LEDPositionsBottomLeftSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(LEDPositionsBottomGapSetting)))
        ui.gapLEDCount->setValue(pSettings->value(pSettingsMap.value(LEDPositionsBottomGapSetting)).toInt());
    if (pSettings->contains(pSettingsMap.value(LEDPositionsBottomRightSetting)))
        ui.bottomRightLEDCount->setValue(pSettings->value(pSettingsMap.value(LEDPositionsBottomRightSetting)).toInt());

    //Processing: restore each configurable type's parameters generically (no per-field setting
    //keys needed - see ProcessManager::processParameters/setProcessParameters), then restore
    //which rows were enabled (reusing the existing ProcessingSetting)
    const QMetaEnum processTypeEnum = QMetaEnum::fromType<ImageProcess::ProcessType>();
    pSettings->beginGroup(QStringLiteral("ProcessParameters"));
    for (int i = 0; i < processTypeEnum.keyCount(); ++i)
    {
        ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(processTypeEnum.value(i));
        pSettings->beginGroup(QString::fromLatin1(processTypeEnum.key(i)));
        if (!pSettings->allKeys().isEmpty())
        {
            QVariantMap parameters;
            for (const QString &key : pSettings->allKeys())
                parameters.insert(key, pSettings->value(key));
            pProcessManager->setProcessParameters(type, parameters);
        }
        pSettings->endGroup();
    }
    pSettings->endGroup();

    if (pSettings->contains(pSettingsMap.value(ProcessingSetting)))
    {
        QStringList enabledProcessNames = pSettings->value(pSettingsMap.value(ProcessingSetting)).toStringList();

        for (int i = 0; i < ui.processingList->count(); ++i)
        {
            QListWidgetItem *item = ui.processingList->item(i);
            ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(item->data(Qt::UserRole).toInt());

            if (type == ImageProcess::LinearScale)
            {
                //The "Scale" row's checkbox is keyed by LinearScale, but the saved enabled set
                //names whichever of the 3 interpolation modes was actually active
                bool scaleFound = false;
                for (ImageProcess::ProcessType scaleType : {ImageProcess::SampleScale, ImageProcess::LinearScale, ImageProcess::CubicScale})
                {
                    if (enabledProcessNames.contains(QString::fromLatin1(processTypeEnum.valueToKey(scaleType))))
                    {
                        pActiveScaleType = scaleType;
                        scaleFound = true;
                        break;
                    }
                }
                item->setCheckState(scaleFound ? Qt::Checked : Qt::Unchecked);
            }
            else
            {
                bool enabled = enabledProcessNames.contains(QString::fromLatin1(processTypeEnum.valueToKey(type)));
                item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
            }
        }

        //Re-populate the currently shown config page now that parameters/mode may have changed
        if (ui.processingList->currentItem())
            populateProcessConfigPage(static_cast<ImageProcess::ProcessType>(ui.processingList->currentItem()->data(Qt::UserRole).toInt()));
    }
}

//Write the current settings to platform specific place
void AmbiLED::writeSettings()
{
    pSettings->setValue(pSettingsMap.value(SerialPortSetting), ui.comPortComboBox->currentText());
    pSettings->setValue(pSettingsMap.value(RefreshRateSetting), ui.refreshRateComboBox->currentText());
    pSettings->setValue(pSettingsMap.value(BrightnessSetting), ui.brightnessSlider->value());
    pSettings->setValue(pSettingsMap.value(VisualiseSetting), ui.visualiseCheckBox->isChecked());
    pSettings->setValue(pSettingsMap.value(CaptureMethodSetting), ui.captureComboBox->currentText());
    pSettings->setValue(pSettingsMap.value(AveragingMethodSetting), ui.averageMethodComboBox->currentText());
    pSettings->setValue(pSettingsMap.value(ColourCorrectionPresetSetting), ui.colourTemperatureComboBox->currentText());
    pSettings->setValue(pSettingsMap.value(ColourCorrectionRedSetting), ui.redColourSlider->value());
    pSettings->setValue(pSettingsMap.value(ColourCorrectionGreenSetting), ui.greenColourSlider->value());
    pSettings->setValue(pSettingsMap.value(ColourCorrectionBlueSetting), ui.blueColourSlider->value());

    pSettings->setValue(pSettingsMap.value(LEDPositionsTopSetting), ui.topLEDCount->value());
    pSettings->setValue(pSettingsMap.value(LEDPositionsLeftSetting), ui.leftLEDCount->value());
    pSettings->setValue(pSettingsMap.value(LEDPositionsRightSetting), ui.rightLEDCount->value());
    pSettings->setValue(pSettingsMap.value(LEDPositionsBottomLeftSetting), ui.bottomLeftLEDCount->value());
    pSettings->setValue(pSettingsMap.value(LEDPositionsBottomGapSetting), ui.gapLEDCount->value());
    pSettings->setValue(pSettingsMap.value(LEDPositionsBottomRightSetting), ui.bottomRightLEDCount->value());

    //Processing: persist which rows are enabled (reusing the existing ProcessingSetting), and
    //every configurable type's current parameters generically - see readSettings()
    const QMetaEnum processTypeEnum = QMetaEnum::fromType<ImageProcess::ProcessType>();

    QStringList enabledProcessNames;
    for (int i = 0; i < ui.processingList->count(); ++i)
    {
        QListWidgetItem *item = ui.processingList->item(i);
        if (item->checkState() != Qt::Checked)
            continue;

        ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(item->data(Qt::UserRole).toInt());
        if (type == ImageProcess::LinearScale)
            type = pActiveScaleType;
        enabledProcessNames.append(QString::fromLatin1(processTypeEnum.valueToKey(type)));
    }
    pSettings->setValue(pSettingsMap.value(ProcessingSetting), enabledProcessNames);

    pSettings->beginGroup(QStringLiteral("ProcessParameters"));
    for (int i = 0; i < processTypeEnum.keyCount(); ++i)
    {
        ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(processTypeEnum.value(i));
        pSettings->beginGroup(QString::fromLatin1(processTypeEnum.key(i)));
        const QVariantMap parameters = pProcessManager->processParameters(type);
        for (auto it = parameters.constBegin(); it != parameters.constEnd(); ++it)
            pSettings->setValue(it.key(), it.value());
        pSettings->endGroup();
    }
    pSettings->endGroup();
}

//===== SERIAL MANAGER SLOTS FOR SIGNALS =====
//Slot to get data from serial port
void AmbiLED::serialManagerSerialDataRead(QByteArray data)
{
    //Serial port has responded with data read, let's read it
	QDataStream stream(data);
	stream.setByteOrder(QDataStream::LittleEndian);
	quint32 readBytes;
	stream >> readBytes;

    if (pDebugMode)
    {
        QString str = QString(data.toHex(':'));
        ui.debugLabel->setText(tr("#%1 returned (R:%2 W:%3) - Raw (%4)").arg(++pReadCounter).arg(readBytes).arg(pSerialBytesWritten).arg(str));
    }
    else {
        ui.debugLabel->setText(tr("#%1 returned (R:%2 W:%3)").arg(++pReadCounter).arg(readBytes).arg(pSerialBytesWritten));
    }

	pSerialBytesWritten = 0;
}

//Serial port has written some data
void AmbiLED::serialManagerSerialDataWritten(int bytes)
{
    pSerialBytesWritten += bytes;
}

//Serial port has replied that it's ready
void AmbiLED::serialManagerReadyForTransmit()
{
	pSerialPortReady = true;
}

//Hardware status has changed
void AmbiLED::serialManagerDeviceStatusChanged(bool status)
{
    //Device is now connected
    if (status)
    {
        ui.deviceStatusLabel->setText(tr("Connected"));
        QIcon connectedIcon(pConnectedPixmap);
        ui.deviceStatusColourLabel->setIcon(connectedIcon);
        ui.deviceStatusColourLabel->setIconSize(pConnectedPixmap.rect().size());
        ui.deviceStatusColourLabel->setFixedSize(pConnectedPixmap.rect().size());
    }
    //Device is disconnected
    else
    {
        ui.deviceStatusLabel->setText(tr("Disconnected"));
        QIcon disconnectedIcon(pDisconnectedPixmap);
        ui.deviceStatusColourLabel->setIcon(disconnectedIcon);
        ui.deviceStatusColourLabel->setIconSize(pDisconnectedPixmap.rect().size());
        ui.deviceStatusColourLabel->setFixedSize(pDisconnectedPixmap.rect().size());
    }
}

//Different lux value has been received
void AmbiLED::serialManagerLuxValueChanged()
{
    ui.brightnessSlider->setValue(pLeds->transformLuxToBrightness(pSerialManager->getLux()));
}

//Serial port failure (error message)
void AmbiLED::serialManagerFailed(QString message)
{
    ui.debugLabel->setText(message);
}

//===== UI SLOTS FOR SIGNALS =====
//Clicked on system tray
//void uiTrayClicked(QSystemTrayIcon::ActivationReason reason);

//Toggle screen capture (fired by either the tray action or the main window checkbox)
void AmbiLED::uiScreenCaptureToggled(bool enabled)
{
    pCaptureEnabled = enabled;
    pScreenManager->setEnabled(pCaptureEnabled);

    //Keep the tray action and the main window checkbox in sync with each other
    QSignalBlocker suspendActionBlocker(pSuspendAction);
    pSuspendAction->setChecked(pCaptureEnabled);
    QSignalBlocker captureCheckBoxBlocker(ui.captureCheckBox);
    ui.captureCheckBox->setChecked(pCaptureEnabled);
}

//Toggle debug mode
void AmbiLED::uiDebugModeToggled()
{
    pDebugMode = ui.debugCheckBox->isChecked();
    pScreenManager->setTestPatternGeneration(pDebugMode);
}

//Visualise fullscreen mode toggled
void AmbiLED::uiFullscreenModeToggled()
{
    pFullscreenMode = ui.visualiseCheckBox->isChecked();

    if (!pFullscreenMode)
    {
        //If disabling, reset the label
        ui.screenLabel->setPixmap(QPixmap());
        QString placeHolderStyle = tr("QLabel { background-color : black; }");
        ui.screenLabel->setStyleSheet(placeHolderStyle);
    }
}

//Capture refresh rate changed
void AmbiLED::uiRefreshRateChanged(int index)
{
    pScreenManager->setFrameRate(ui.refreshRateComboBox->itemData(index).toInt());
}

//Slider has been moved to change brightness
void AmbiLED::uiBrightnessChanged(int value)
{
    pLeds->setBrightness(static_cast<qreal>(value) / 100.0);
}

//Capture mode has been changed
void AmbiLED::uiCaptureModeChanged(int index)
{
    pScreenManager->setCaptureMode(static_cast<ScreenCapture::CaptureMode>(ui.captureComboBox->itemData(index).toInt()));
}

//LED positions have changed
void AmbiLED::uiLedPositionsChanged()
{
    LEDPositions p;
    p.t = ui.topLEDCount->value();
    p.l = ui.leftLEDCount->value();
    p.r = ui.rightLEDCount->value();
    p.bl = ui.bottomLeftLEDCount->value();
    p.br = ui.bottomRightLEDCount->value();
    p.bg = ui.gapLEDCount->value();

    pLeds->setPositions(p);
}

//User has selected a different serial port
void AmbiLED::uiSerialPortChanged(int index)
{
    Q_UNUSED(index);

	QString	selectedPortName = ui.comPortComboBox->currentText();
    pSerialManager->changeSerialPort(selectedPortName);
}

//Colour correction has changed
void AmbiLED::uiColourTempSlidersChanged()
{
    pLeds->setColourTemperature(ui.redColourSlider->value(), ui.greenColourSlider->value(), ui.blueColourSlider->value());
}

//Preset colour correction has changed
void AmbiLED::uiColourTempComboChanged(int index)
{
    int selectedValue = ui.colourTemperatureComboBox->itemData(index).toInt();
	if (selectedValue)
	{
        pLeds->setColourTemperature(selectedValue);

		//Disable sliders
		ui.redColourSlider->setEnabled(false);
		ui.greenColourSlider->setEnabled(false);
		ui.blueColourSlider->setEnabled(false);


		//Stop slider signals
		ui.redColourSlider->blockSignals(true);
		ui.greenColourSlider->blockSignals(true);
		ui.blueColourSlider->blockSignals(true);

		//Set sliders
        ui.redColourSlider->setValue((selectedValue >> 16) & 0xFF);
        ui.greenColourSlider->setValue((selectedValue >> 8) & 0xFF);
        ui.blueColourSlider->setValue((selectedValue) & 0xFF);

		ui.redColourLabel->setText(tr("%1").arg(ui.redColourSlider->value()));
		ui.greenColourLabel->setText(tr("%1").arg(ui.greenColourSlider->value()));
		ui.blueColourLabel->setText(tr("%1").arg(ui.blueColourSlider->value()));

		//Reset slider signals
		ui.redColourSlider->blockSignals(false);
		ui.greenColourSlider->blockSignals(false);
		ui.blueColourSlider->blockSignals(false);
	}
	else
	{
		//Custom selected
		//Enable sliders
		ui.redColourSlider->setEnabled(true);
		ui.greenColourSlider->setEnabled(true);
		ui.blueColourSlider->setEnabled(true);
	}
}

//Different average method has been selected
void AmbiLED::uiAverageMethodComboChanged(int index)
{
    ImageTransform::TransformType transformType = static_cast<ImageTransform::TransformType>(ui.averageMethodComboBox->itemData(index).toInt());
    pProcessManager->setTransform(transformType);
}

//A process's checkbox was toggled in the processing list (enable/disable)
void AmbiLED::uiProcessListItemChanged(QListWidgetItem *item)
{
    ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(item->data(Qt::UserRole).toInt());
    bool enabled = item->checkState() == Qt::Checked;

    //The "Scale" row is keyed by LinearScale but always acts on whichever interpolation mode
    //is currently selected (see pActiveScaleType)
    if (type == ImageProcess::LinearScale)
        type = pActiveScaleType;

    if (enabled)
        pProcessManager->enableProcess(type);
    else
        pProcessManager->disableProcess(type);
}

//The current item in the processing list changed - switch which config page is shown
void AmbiLED::uiProcessListCurrentItemChanged(QListWidgetItem *current)
{
    if (!current)
        return;

    ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(current->data(Qt::UserRole).toInt());
    QWidget *page = pProcessConfigPages.value(type);
    if (!page)
        return;

    populateProcessConfigPage(type);
    ui.processingConfigStack->setCurrentWidget(page);
}

//Any config widget on the currently shown process config page changed value
void AmbiLED::uiProcessConfigValueChanged()
{
    QWidget *page = ui.processingConfigStack->currentWidget();
    ImageProcess::ProcessType type = static_cast<ImageProcess::ProcessType>(page->property("processType").toInt());

    if (type == ImageProcess::LinearScale)
        type = pActiveScaleType;

    QVariantMap parameters;
    const QList<QSpinBox *> spinBoxes = page->findChildren<QSpinBox *>();
    for (QSpinBox *spinBox : spinBoxes)
    {
        const QString key = spinBox->property("parameterKey").toString();
        if (!key.isEmpty())
            parameters.insert(key, spinBox->value());
    }

    pProcessManager->setProcessParameters(type, parameters);
}

//Scale's interpolation mode combo changed - not a process parameter, but a choice of which
//ProcessType is the active one. Carries the current percentage over to the newly chosen type,
//and swaps which one is actually enabled if Scale is currently on.
void AmbiLED::uiScaleModeChanged(int index)
{
    ImageProcess::ProcessType newType = static_cast<ImageProcess::ProcessType>(pScaleModeComboBox->itemData(index).toInt());
    if (newType == pActiveScaleType)
        return;

    pProcessManager->setProcessParameters(newType, pProcessManager->processParameters(pActiveScaleType));

    if (pProcessManager->isProcessEnabled(pActiveScaleType))
    {
        pProcessManager->disableProcess(pActiveScaleType);
        pProcessManager->enableProcess(newType);
    }

    pActiveScaleType = newType;
}

//Builds the processing list rows and their generic config pages, then shows the first one.
//Called from the constructor (not setupGUI()) since it needs pProcessManager to already exist.
void AmbiLED::setupProcessingList()
{
    pActiveScaleType = ImageProcess::LinearScale;
    if (pProcessManager->isProcessEnabled(ImageProcess::SampleScale))
        pActiveScaleType = ImageProcess::SampleScale;
    else if (pProcessManager->isProcessEnabled(ImageProcess::CubicScale))
        pActiveScaleType = ImageProcess::CubicScale;

    auto addRow = [this](const QString &text, ImageProcess::ProcessType type, bool checked)
    {
        QListWidgetItem *item = new QListWidgetItem(text);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, type);
        ui.processingList->addItem(item);

        QWidget *page = createProcessConfigPage(type);
        pProcessConfigPages.insert(type, page);
        ui.processingConfigStack->addWidget(page);
    };

    addRow(tr("Remove black bars"), ImageProcess::BlackBarRemoval, pProcessManager->isProcessEnabled(ImageProcess::BlackBarRemoval));
    addRow(tr("Reduce flicker (low-pass filter)"), ImageProcess::LowPassFilter, pProcessManager->isProcessEnabled(ImageProcess::LowPassFilter));
    addRow(tr("Scale down (reduce resolution)"), ImageProcess::LinearScale, pProcessManager->isProcessEnabled(pActiveScaleType));

    connect(ui.processingList, &QListWidget::itemChanged, this, &AmbiLED::uiProcessListItemChanged);
    connect(ui.processingList, &QListWidget::currentItemChanged, this, &AmbiLED::uiProcessListCurrentItemChanged);

    ui.processingList->setCurrentRow(0);
}

//Builds a config page (one spin box per IntValue ParameterSpec) for the given process type
QWidget *AmbiLED::createProcessConfigPage(ImageProcess::ProcessType type)
{
    QWidget *page = new QWidget();
    page->setProperty("processType", static_cast<int>(type));
    QFormLayout *layout = new QFormLayout(page);

    //Scale also needs to choose which interpolation algorithm is active - not a process
    //parameter, but a choice between three ProcessTypes (see uiScaleModeChanged)
    if (type == ImageProcess::LinearScale)
    {
        pScaleModeComboBox = new QComboBox(page);
        pScaleModeComboBox->addItem(tr("Nearest (fastest)"), ImageProcess::SampleScale);
        pScaleModeComboBox->addItem(tr("Linear"), ImageProcess::LinearScale);
        pScaleModeComboBox->addItem(tr("Cubic (smoothest)"), ImageProcess::CubicScale);
        connect(pScaleModeComboBox, &QComboBox::currentIndexChanged, this, &AmbiLED::uiScaleModeChanged);
        layout->addRow(tr("Interpolation"), pScaleModeComboBox);
    }

    const QList<ImageProcess::ParameterSpec> specs = ProcessManager::parameterSpecsForType(type);
    for (const ImageProcess::ParameterSpec &spec : specs)
    {
        if (spec.kind != ImageProcess::ParameterSpec::IntValue)
            continue;

        QSpinBox *spinBox = new QSpinBox(page);
        spinBox->setRange(spec.minimum, spec.maximum);
        spinBox->setSingleStep(spec.step);
        spinBox->setSuffix(spec.suffix);
        spinBox->setProperty("parameterKey", spec.key);
        connect(spinBox, &QSpinBox::valueChanged, this, &AmbiLED::uiProcessConfigValueChanged);
        layout->addRow(spec.label, spinBox);
    }

    return page;
}

//Fills in the given process type's config page widgets from ProcessManager's current values
void AmbiLED::populateProcessConfigPage(ImageProcess::ProcessType type)
{
    QWidget *page = pProcessConfigPages.value(type);
    if (!page)
        return;

    ImageProcess::ProcessType parameterType = type;
    if (type == ImageProcess::LinearScale)
    {
        parameterType = pActiveScaleType;
        if (pScaleModeComboBox)
        {
            QSignalBlocker blocker(pScaleModeComboBox);
            pScaleModeComboBox->setCurrentIndex(pScaleModeComboBox->findData(pActiveScaleType));
        }
    }

    const QVariantMap parameters = pProcessManager->processParameters(parameterType);
    const QList<QSpinBox *> spinBoxes = page->findChildren<QSpinBox *>();
    for (QSpinBox *spinBox : spinBoxes)
    {
        const QString key = spinBox->property("parameterKey").toString();
        if (parameters.contains(key))
        {
            QSignalBlocker blocker(spinBox);
            spinBox->setValue(parameters.value(key).toInt());
        }
    }
}

//Clicked on system tray
void AmbiLED::uiTrayClicked(QSystemTrayIcon::ActivationReason reason)
{
    //If you click or double click the system tray icon, show the configuration screen
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
    {
        this->show();
        this->setWindowState(this->windowState() & (~Qt::WindowMinimized | Qt::WindowActive));
    }
}

//Quit was selected from the tray menu
void AmbiLED::uiQuitTriggered()
{
    writeSettings();
    qApp->quit();
}

//===== SCREEN MANAGER SLOTS FOR SIGNALS =====
//Final processed image is ready
void AmbiLED::screenManagerReadyFrame(QImage screenImage)
{
    Q_ASSERT(!screenImage.isNull());
    PerfTimer timer(pGuiFrameStats);

    //Display fullscreen on UI
    if (pFullscreenMode && previewDue(pNextScreenPreviewMs))
        displayFullScreen(screenImage);

    //Reset idle mode if currently activated - capture has recovered, so the failure message
    //screenManagerFailed() left in the debug label no longer applies
    if (pCaptureIdleMode)
    {
        disableIdleMode();
        ui.debugLabel->clear();
    }

    //Send the captured frame for processing
    emit processManagerStartProcess(screenImage, pLeds->getStripSizes());
}

//If capture has failed, slow down the timer
void AmbiLED::screenManagerFailed(QString message)
{
    ui.debugLabel->setText(message);

	if (!pCaptureIdleMode)
        enableIdleMode();
}

//The Wayland backend's portal restore_token changed - persist it so future launches skip the share dialog
void AmbiLED::screenManagerWaylandRestoreTokenUpdated(QString token)
{
    pSettings->setValue(pSettingsMap.value(WaylandRestoreTokenSetting), token);
    pSettings->sync();
}

//===== PROCESS MANAGER SLOTS FOR SIGNALS =====
//Processing has been completed
void AmbiLED::processManagerReadyProcess(ImageStrips strips)
{
    Q_ASSERT(!strips.isNull());
    PerfTimer timer(pGuiStripsStats);

    //Add the frame time - here rather than in the (throttled) preview, so it reflects every processed frame
    qint64 elapsed = pElapsedFrameTimer->elapsed();
    while (pFrameAverageQueue.count() < 10) //Smooth the frame counter
        pFrameAverageQueue.enqueue(elapsed);

    pFrameAverageQueue.enqueue(elapsed);
    pFrameAverageQueue.dequeue();
    pElapsedFrameTimer->restart();

    //Display the strips on the UI
    bool showPreview = previewDue(pNextStripsPreviewMs);
    if (showPreview)
        displayStrips(strips);

    //Update the LED representation of the strip
    pLeds->updateLED(strips);

    //Send the strip information to the serial port (if it's ready)
    QImage stripImage = pLeds->getStrip();

    if (pSerialPortReady)
    {
        //Serial port is ready
        pSerialPortReady = false;
        qint64 elapsed = pElapsedSerialTimer->elapsed();
        while (pSerialAverageQueue.count() < 10) //Smoothing
            pSerialAverageQueue.enqueue(elapsed);

        pSerialAverageQueue.enqueue(elapsed);
        pSerialAverageQueue.dequeue();

        pElapsedSerialTimer->restart();
        emit serialManagerWriteLEDImage(stripImage);
    }

    //Update the UI LED image at the bottom
    if (showPreview)
        displayLEDStrip(stripImage);
}

//Processing failure
void AmbiLED::processManagerFailed(QString message)
{
    Q_UNUSED(message);

    if (!pCaptureIdleMode)
        enableIdleMode();
}

//===== CLASS FUNCTIONS =====
//Display image on label
void AmbiLED::displayStrips(ImageStrips outLines)
{
    //Early out if not visible
    if (!this->isVisible())
        return;

    //Ensure everything is valid
    Q_ASSERT(!outLines.isNull());
    if (outLines.isNull())
        return;

    //Generate pixmaps from the images
    QPixmap top = QPixmap::fromImage(outLines.topLine.scaled(ui.topImage->width(), ui.topImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    QPixmap bottom = QPixmap::fromImage(outLines.bottomLine.scaled(ui.bottomImage->width(), ui.bottomImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    QPixmap left = QPixmap::fromImage(outLines.leftLine.scaled(ui.leftImage->width(), ui.leftImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    QPixmap right = QPixmap::fromImage(outLines.rightLine.scaled(ui.rightImage->width(), ui.rightImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));

    //Draw arrows to show direction of LED data shift
    PixmapUtils::drawArrow(top, PixmapUtils::RightArrow, Qt::white, ui.topImage->width() / 2 + 10, -1, ui.topImage->width() / 2 + 10, "Strip #1");
    PixmapUtils::drawArrow(top, PixmapUtils::LeftArrow, Qt::white, ui.topImage->width() / 2 + 10, -1, ui.topImage->width() / 2 + 10, "Strip #2");
    PixmapUtils::drawArrow(left, PixmapUtils::UpArrow, Qt::white, 0, -1, -1, "Strip #1");
    PixmapUtils::drawArrow(right, PixmapUtils::UpArrow, Qt::white, 0, -1, -1, "Strip #2");

    LEDPositions currentPositions = pLeds->getPositions();
    int bottomLeftIndex = (ui.bottomImage->width() * currentPositions.bl) / currentPositions.getBottomCount();
    int bottomRightIndex = (ui.bottomImage->width() * currentPositions.br) / currentPositions.getBottomCount();
    PixmapUtils::drawArrow(bottom, PixmapUtils::LeftArrow, Qt::white, 0, -1, bottomLeftIndex, "Strip #1");
    PixmapUtils::drawArrow(bottom, PixmapUtils::RightArrow, Qt::white, 0, -1, bottomRightIndex, "Strip #2");

    ui.topImage->setPixmap(top);
    ui.bottomImage->setPixmap(bottom);
    ui.leftImage->setPixmap(left);
    ui.rightImage->setPixmap(right);
}

//True (and the deadline advanced) if a preview redraw is due now. The deadline advances by whole
//periods rather than restarting from "now", so frames arriving with jitter (e.g. every ~16.7 ms at
//60 fps) still average PreviewFrameRate instead of rounding down to every 4th frame (15 Hz).
bool AmbiLED::previewDue(qint64 &nextDueMs)
{
    if (!pPreviewClock.isValid())
        pPreviewClock.start();

    qint64 now = pPreviewClock.elapsed();
    if (now < nextDueMs)
        return false;

    const qint64 periodMs = 1000 / PreviewFrameRate;
    //After a gap (e.g. idle, or the window was hidden), restart the cadence instead of catching up
    nextDueMs = (now - nextDueMs >= periodMs) ? now + periodMs : nextDueMs + periodMs;
    return true;
}

//Display full screen on label
void AmbiLED::displayFullScreen(QImage image)
{
    //Early out if not visible
    if (!this->isVisible())
        return;

    Q_ASSERT(!image.isNull());

    //Create pixmap and set the label to it
    QPixmap screen = QPixmap::fromImage(image.scaled(ui.screenLabel->width(), ui.screenLabel->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
    PixmapUtils::drawOffsetLines(screen, Qt::red);
    ui.screenLabel->setPixmap(screen);
}

//Update the UI LED image at the bottom
void AmbiLED::displayLEDStrip(QImage image)
{
    //Early out if not visibile
    if (!this->isVisible())
        return;

    Q_ASSERT(!image.isNull());

    //Create pixmap and set the label to it
    ui.ledStrip->setPixmap(QPixmap::fromImage(image.scaled(ui.ledStrip->width(), ui.ledStrip->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation)));
}

//Update UI elements that require it
void AmbiLED::updateUIElements()
{
    //Calculate average serial latency
	qreal averageSerialLatency = 0;
    for (int i = 0; i < pSerialAverageQueue.count(); ++i)
		averageSerialLatency += pSerialAverageQueue.at(i);
	averageSerialLatency /= pSerialAverageQueue.count();

    //Calculate average frame latency
	qreal averageFrameLatency = 0;
    for (int i = 0; i < pFrameAverageQueue.count(); ++i)
		averageFrameLatency += pFrameAverageQueue.at(i);
	averageFrameLatency /= pFrameAverageQueue.count();

    //Update serial and frame labels
	ui.serialLabel->setText(tr("Serial latency: %1 msec (%2 Hz)").arg(averageSerialLatency, 0, 'f', 2).arg(1000.0 / averageSerialLatency, 0, 'f', 2));
	ui.frameLabel->setText(tr("Latency: %1 msec (%2 Hz)").arg(averageFrameLatency, 0, 'f', 2).arg(1000.0 / averageFrameLatency, 0, 'f', 2));
}

//Show popup with device firmware and information
void AmbiLED::showDeviceFirmware()
{
    if (!pSerialManager->connected())
		return;

    QMessageBox::information(this, tr("Device information"), tr("%1").arg(pSerialManager->getFirmwareVersion()), QMessageBox::Ok);
}

//Set idle mode
void AmbiLED::setIdleMode(bool state)
{
    pCaptureIdleMode = state;

    //Set relevant frame rate
    if (pCaptureIdleMode)
        pScreenManager->setFrameRate(1);
    else
        pScreenManager->setFrameRate(ui.refreshRateComboBox->currentData().toInt());
}

//Override that gets called when window is closing
void AmbiLED::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    //Try to write current settings
    writeSettings();
}

//Override that gets called when window is minimised
void AmbiLED::hideEvent(QHideEvent* event)
{
    //Don't handle when it goes out of focus etc.
    if (!event->spontaneous())
        return;

    //Show notification once
    if (!pNotificationShown)
    {
        pTray->showMessage(tr("AmbiLED Control"), tr("AmbiLED Control has been minimised to system tray"), QSystemTrayIcon::Information, 500);
        pNotificationShown = true;
    }
    hide();
}

//===== UI SETUP FUNCTIONS =====
void AmbiLED::createActions()
{
    //Create all the relevant actions and their signals
    pQuitAction = new QAction(tr("&Quit"), this);
    connect(pQuitAction, &QAction::triggered, this, &AmbiLED::uiQuitTriggered);

    pConfigureAction = new QAction(tr("&Configure"), this);
    connect(pConfigureAction, &QAction::triggered, this, &AmbiLED::showNormal);

    pSuspendAction = new QAction(tr("Capture &Screen"), this);
    pSuspendAction->setCheckable(true);
    pSuspendAction->setChecked(true);
    connect(pSuspendAction, &QAction::toggled, this, &AmbiLED::uiScreenCaptureToggled);
}

void AmbiLED::createTrayIcon()
{
    Q_ASSERT(pQuitAction && pConfigureAction && pSuspendAction);

    //Create the system tray menu
	QMenu *trayMenu = new QMenu(this);
    trayMenu->addAction(pSuspendAction);
    trayMenu->addAction(pConfigureAction);
    trayMenu->addAction(pQuitAction);

    pTray = new QSystemTrayIcon(this);
    pTray->setToolTip(tr("AmbiLED v%1").arg(VERSION_STR));
    pTray->setIcon(QIcon(":/AmbiLED/monitor_icon"));
    pTray->setContextMenu(trayMenu);

    connect(pTray, &QSystemTrayIcon::activated, this, &AmbiLED::uiTrayClicked);
}

void AmbiLED::createPixmaps()
{
	//Create transparent surfaces
    pConnectedPixmap = QPixmap(16,16);
    pConnectedPixmap.fill(Qt::transparent);
    pDisconnectedPixmap = QPixmap(16, 16);
    pDisconnectedPixmap.fill(Qt::transparent);

	QPen pen;
	pen.setColor(Qt::black);
	pen.setWidth(1);

	QBrush brush;
	brush.setStyle(Qt::SolidPattern);

	//Create connected icon (green circle)
	brush.setColor(Qt::green);
    QPainter connectedPainter(&pConnectedPixmap);
	connectedPainter.setPen(pen);
	connectedPainter.setBrush(brush);

	connectedPainter.drawEllipse(0, 0, 15, 15);
	
	//Create disconnected icon (red circle)
	brush.setColor(Qt::red);
    QPainter disconnectedPainter(&pDisconnectedPixmap);
	disconnectedPainter.setPen(pen);
	disconnectedPainter.setBrush(brush);

	disconnectedPainter.drawEllipse(0, 0, 15, 15);
}


