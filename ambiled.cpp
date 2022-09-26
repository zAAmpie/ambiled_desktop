#include <QtWidgets>
#include "ambiled.h"



AmbiLED::AmbiLED(QWidget *parent)
    : QMainWindow(parent),     //Set up local variables
      pReadCounter(0),
      pFrameCounter(0),
      pSerialBytesWritten(0),
      pNotificationShown(false),
      pCaptureIdleMode(false)
{
    //Register metatypes to use in signal/slot system
    qRegisterMetaType<ImageStrips>("ImageStripsInput");
	qRegisterMetaType<LEDPositions>("LEDPositions");

    //Create map of enum
    createSettingsMap();

	//Create new settings object
	pSettings = new QSettings("zAAm", "AmbiLED");

    //Set up serial port options
    serialDeviceStatusChanged(false);

	foreach (QSerialPortInfo info, QSerialPortInfo::availablePorts())
	{
		ui.comPortComboBox->addItem(info.portName().isEmpty() ? info.description() : info.portName());
	}
    ui.comPortComboBox->setCurrentIndex(0);

	//Create serial port object
    serialPort = new Serial(ui.comPortComboBox->currentText());

    connect(this, &AmbiLED::writeLEDImage, serialPort, &Serial::writeLEDImage);
    connect(serialPort, &Serial::readyForTransmit, this, &AmbiLED::serialPortReady);
    connect(serialPort, &Serial::serialDataRead, this, &AmbiLED::serialPortDataRead);
    connect(serialPort, &Serial::serialDataWritten, this, &AmbiLED::serialPortDataWritten);
    connect(this, &AmbiLED::updateSerialPort, serialPort, &Serial::changeSerialPort);
    connect(serialPort, &Serial::deviceStatusChanged, this, &AmbiLED::serialDeviceStatusChanged);
    //connect(serialPort, &Serial::luxValueChanged, this, &AmbiLED::luxValueChanged);

	pSerialPortReady = true;

    //Create LED object
    leds = new LEDS();

	//Create screen capture object
	capture = new ScreenCapture(this);
    connect(this, &AmbiLED::updateScreenCapture, capture, &ScreenCapture::captureScreen);
    connect(capture, &ScreenCapture::returnCapturedStrips, this, &AmbiLED::displayStrips);
    connect(capture, &ScreenCapture::returnFullScreen, this, &AmbiLED::displayFullScreen);
    connect(capture, &ScreenCapture::captureFailed, this, &AmbiLED::captureFailed);

    //Move capturing of the screen to another thread. Will need to see if that's possible with X11
    pCaptureThread = new ExecThread();
    capture->moveToThread(pCaptureThread);
    pCaptureThread->start();

    //Create screen update timer
    pScreenUpdateTimer = new QTimer();
    pScreenUpdateTimer->setTimerType(Qt::PreciseTimer);
    connect(pScreenUpdateTimer, &QTimer::timeout, this, &AmbiLED::captureScreen);
    pScreenUpdateTimer->start(ui.refreshRateComboBox->currentData().toInt());

	//Create elapsed timer for FPS
	elapsedTimer = new QElapsedTimer();
	elapsedTimer->start();

    //Create elapsed timer for Serial writes
	elapsedSerialTimer = new QElapsedTimer();
	elapsedSerialTimer->start();

    //Notify the relevant objects of the LED positions
    ledPositionsChanged();

    //Update GUI with relevant statistics
	uiUpdateTimer = new QTimer();
	uiUpdateTimer->setTimerType(Qt::PreciseTimer);
	connect(uiUpdateTimer, SIGNAL(timeout()), this, SLOT(updateUIElements()));
	uiUpdateTimer->start(200); //Update at a rate of 5Hz

    //Read and populate previous settings
	readSettings();
}

AmbiLED::~AmbiLED()
{
    pCaptureThread->quit();
    if (pCaptureThread->wait(5000))
        delete pCaptureThread;
    else
    {
        pCaptureThread->terminate();
        delete pCaptureThread;
    }
	
	//Free frame buffer data
	delete capture;
	delete serialPort;
    delete leds;
}

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
    connect(ui.comPortComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(serialPortChanged(int)));

    //Enumerate refresh rate options
    ui.refreshRateComboBox->addItem(tr("Maximum"), 1);
    ui.refreshRateComboBox->addItem(tr("60 Hz"), 16);
    ui.refreshRateComboBox->addItem(tr("50 Hz"), 20);
    ui.refreshRateComboBox->addItem(tr("30 Hz"), 33);
    ui.refreshRateComboBox->addItem(tr("25 Hz"), 40);
    ui.refreshRateComboBox->addItem(tr("24 Hz"), 42);
    ui.refreshRateComboBox->addItem(tr("20 Hz"), 50);
    ui.refreshRateComboBox->addItem(tr("15 Hz"), 67);
    ui.refreshRateComboBox->addItem(tr("10 Hz"), 100);
    ui.refreshRateComboBox->addItem(tr("5 Hz"), 200);
    ui.refreshRateComboBox->addItem(tr("1 Hz"), 1000);
    ui.refreshRateComboBox->setCurrentIndex(2);
    connect(ui.refreshRateComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshRateChanged(int)));

    //Set up capture mode
    ui.captureComboBox->addItem(tr("X11"), ScreenCapture::X11Mode);
    ui.captureComboBox->setCurrentIndex(0);
    connect(ui.captureComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(captureModeChanged(int)));

    //Set up brightness slider
    connect(ui.brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(brightnessChanged(int)));
    connect(ui.brightnessSlider, &QSlider::valueChanged, this, [=]() {ui.brightnessValueLabel->setText(tr("%1%").arg(ui.brightnessSlider->value())); });
    ui.brightnessValueLabel->setText(tr("%1%").arg(ui.brightnessSlider->value()));

    //Set up debug connection
    connect(ui.debugCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleDebugMode()));

    //Set up visualise connection
    connect(ui.visualiseCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleFullscreenMode()));

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
    connect(ui.colourTemperatureComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(colourCorrectionComboChanged(int)));

    //Set up custom colour sliders
    connect(ui.redColourSlider, &QSlider::valueChanged, this, [=]() {ui.redColourLabel->setText(QString::number(ui.redColourSlider->value())); });
    connect(ui.greenColourSlider, &QSlider::valueChanged, this, [=]() {ui.greenColourLabel->setText(QString::number(ui.greenColourSlider->value())); });
    connect(ui.blueColourSlider, &QSlider::valueChanged, this, [=]() {ui.blueColourLabel->setText(QString::number(ui.blueColourSlider->value())); });

    connect(ui.redColourSlider, SIGNAL(valueChanged(int)), this, SLOT(colourCorrectionChanged()));
    connect(ui.greenColourSlider, SIGNAL(valueChanged(int)), this, SLOT(colourCorrectionChanged()));
    connect(ui.blueColourSlider, SIGNAL(valueChanged(int)), this, SLOT(colourCorrectionChanged()));

    //Set up averaging mode
    ui.averageMethodComboBox->addItem(tr("No Averaging (Basic/Fastest)"), ScreenCapture::NoAveraging);
    ui.averageMethodComboBox->addItem(tr("Strip Averaging (Good/Fast)"), ScreenCapture::StripAveraging);
    ui.averageMethodComboBox->addItem(tr("Block/Strip Averaging (Nice/Slow)"), ScreenCapture::StripAveraging);
    ui.averageMethodComboBox->addItem(tr("Block Averaging (Great/Slowest)"), ScreenCapture::BlockAveraging);
    connect(ui.averageMethodComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(averageMethodComboChanged(int)));

    //Set up LED positions
    connect(ui.bottomLeftLEDCount, SIGNAL(valueChanged(int)), this, SLOT(ledPositionsChanged()));
    connect(ui.bottomRightLEDCount, SIGNAL(valueChanged(int)), this, SLOT(ledPositionsChanged()));
    connect(ui.gapLEDCount, SIGNAL(valueChanged(int)), this, SLOT(ledPositionsChanged()));
    connect(ui.leftLEDCount, SIGNAL(valueChanged(int)), this, SLOT(ledPositionsChanged()));
    connect(ui.rightLEDCount, SIGNAL(valueChanged(int)), this, SLOT(ledPositionsChanged()));
    connect(ui.topLEDCount, SIGNAL(valueChanged(int)), this, SLOT(ledPositionsChanged()));

    //Set up device firmware label connetion
    connect(ui.deviceStatusColourLabel, SIGNAL(clicked()), this, SLOT(showDeviceFirmware()));

    //Set up save/discard button
    connect(ui.saveConfigButton, SIGNAL(clicked()), this, SLOT(writeSettings()));
    connect(ui.discardConfigButton, SIGNAL(clicked()), this, SLOT(readSettings()));

    //Create tray icon
    createActions();
    createTrayIcon();
    tray->show();

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
		emit updateSerialPort(portName);
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
}

//Slot to get data from serial port
void AmbiLED::serialPortDataRead(QByteArray data)
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

//Serial port has replied that it's ready
void AmbiLED::serialPortReady()
{
	pSerialPortReady = true;
}

//Serial port has written some data
void AmbiLED::serialPortDataWritten(int bytes)
{
	pSerialBytesWritten += bytes;
}

//Capture refresh rate changed
void AmbiLED::refreshRateChanged(int index)
{
	//Set the timer to display at new refresh rate
    pScreenUpdateTimer->start(ui.refreshRateComboBox->itemData(index).toInt());
}

//Capture mode has been changed
void AmbiLED::captureModeChanged(int index)
{
    //Directly set the capture mode (this will not be queued - TODO)
    capture->setCaptureMode(static_cast<ScreenCapture::CaptureMode>(ui.captureComboBox->itemData(index).toInt()));
}

//Slider has been moved to change brightness
void AmbiLED::brightnessChanged(int value)
{
    leds->setBrightness(static_cast<qreal>(value) / 100.0);
}

//User has selected a different serial port
void AmbiLED::serialPortChanged(int)
{
	//QString	selectedPortName = ui.comPortComboBox->itemText(index);
	QString	selectedPortName = ui.comPortComboBox->currentText();
    emit updateSerialPort(selectedPortName);
}

//LED positions have changed
void AmbiLED::ledPositionsChanged()
{
    LEDPositions p;
    p.t = ui.topLEDCount->value();
    p.l = ui.leftLEDCount->value();
    p.r = ui.rightLEDCount->value();
    p.bl = ui.bottomLeftLEDCount->value();
    p.br = ui.bottomRightLEDCount->value();
    p.bg = ui.gapLEDCount->value();

    leds->setPositions(p);
}

//Colour correction has changed
void AmbiLED::colourTemperatureChanged()
{
    leds->setColourTemperature(ui.redColourSlider->value(), ui.greenColourSlider->value(), ui.blueColourSlider->value());
}

//Preset colour correction has changed
void AmbiLED::colourTemperatureComboChanged(int index)
{
    int selectedValue = ui.colourTemperatureComboBox->itemData(index).toInt();
	if (selectedValue)
	{
        leds->setColourTemperature(selectedValue);

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
void AmbiLED::averageMethodComboChanged(int index)
{
    ScreenCapture::AverageMode averageMode = static_cast<ScreenCapture::AverageMode>(ui.averageMethodComboBox->itemData(index).toInt());
    //Set without queueing - TODO
    capture->setAverageMode(averageMode);
}

//Different lux value has been received
void AmbiLED::luxValueChanged()
{
    ui.brightnessSlider->setValue(leds->transformLuxToBrightness(serialPort->getLux()));
}

//Toggle screen capture
void AmbiLED::toggleCapture()
{
	pCaptureScreen = !pCaptureScreen;
	suspendAction->setChecked(pCaptureScreen);
}

//Toggle debug mode
void AmbiLED::toggleDebugMode()
{
    pDebugMode = ui.debugCheckBox->isChecked();
}

//Visualise fullscreen mode toggled
void AmbiLED::toggleFullscreenMode()
{
    //No queueing of this - TODO
    capture->setFullscreenEnabled(ui.visualiseCheckBox->isChecked());
}

//If capture has failed, slow down the timer
void AmbiLED::captureFailed()
{
	if (!pCaptureIdleMode)
	{
		//Enable idle mode (only capture once per second)
		pCaptureIdleMode = true;
        pScreenUpdateTimer->start(1000);
	}
}

void AmbiLED::trayClicked(QSystemTrayIcon::ActivationReason reason)
{
	//If you click or double click the system tray icon, show the configuration screen
	if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
	{
		this->setWindowState(this->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
		this->show();
	}
}

void AmbiLED::captureScreen()
{
	//Capture screen normally and send through to serial
    if (pCaptureScreen && !pDebugMode)
	{
        emit updateScreenCapture(leds->getPositions());
	}

    //If Debug Mode, don't capture screen, rather send the test pattern
    if (pDebugMode)
	{
        QPair<QImage, ImageStrips> output = leds->generateTestPattern();

        sendLEDData(output.first);
        displayStrips(output.second);
		ui.screenLabel->clear();
	}
}

void AmbiLED::displayStrips(ImageStrips outLines)
{
	if (pCaptureIdleMode)
	{
		//Reset idle mode
		pCaptureIdleMode = false;
        pScreenUpdateTimer->start(ui.refreshRateComboBox->currentData().toInt());
	}

	if (this->isVisible())
	{
        if (!outLines.isNull())
        {
            QPixmap top = QPixmap::fromImage(outLines.topLine.scaled(ui.topImage->width(), ui.topImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
            QPixmap bottom = QPixmap::fromImage(outLines.bottomLine.scaled(ui.bottomImage->width(), ui.bottomImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
            QPixmap left = QPixmap::fromImage(outLines.leftLine.scaled(ui.leftImage->width(), ui.leftImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
            QPixmap right = QPixmap::fromImage(outLines.rightLine.scaled(ui.rightImage->width(), ui.rightImage->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));

            //Draw arrows to show direction of LED data shift
            drawArrow(top, AmbiLED::RightArrow, Qt::white, ui.topImage->width() / 2 + 10, -1, ui.topImage->width() / 2 + 10, "Strip #1");
            drawArrow(top, AmbiLED::LeftArrow, Qt::white, ui.topImage->width() / 2 + 10, -1, ui.topImage->width() / 2 + 10, "Strip #2");
            drawArrow(left, AmbiLED::UpArrow, Qt::white, 0, -1, -1, "Strip #1");
            drawArrow(right, AmbiLED::UpArrow, Qt::white, 0, -1, -1, "Strip #2");

            int bottomLeftIndex = (ui.bottomImage->width() * leds->getPositions().bl) / leds->getPositions().getBottomCount();
            int bottomRightIndex = (ui.bottomImage->width() * leds->getPositions().br) / leds->getPositions().getBottomCount();
            drawArrow(bottom, AmbiLED::LeftArrow, Qt::white, 0, -1, bottomLeftIndex, "Strip #1");
            drawArrow(bottom, AmbiLED::RightArrow, Qt::white, 0, -1, bottomRightIndex, "Strip #2");

            ui.topImage->setPixmap(top);
            ui.bottomImage->setPixmap(bottom);
            ui.leftImage->setPixmap(left);
            ui.rightImage->setPixmap(right);
        }

		//Display delay
        qint64 elapsed = elapsedTimer->elapsed();
		while (pFrameAverageQueue.count() < 10)
		{
			pFrameAverageQueue.enqueue(elapsed);
		}
		pFrameAverageQueue.enqueue(elapsed);
		pFrameAverageQueue.dequeue();
		//ui.frameLabel->setText(tr("Latency: %1 msec (%2/s)").arg(elapsed).arg(1000 / elapsed));
		elapsedTimer->restart();
	}
}

void AmbiLED::displayFullScreen(QImage image)
{
	if (this->isVisible())
	{
        if (ui.visualiseCheckBox->isChecked() && !pDebugMode)
		{
            QPixmap screen = QPixmap::fromImage(image.scaled(ui.screenLabel->width(), ui.screenLabel->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation));
			//if (ui.stripAverageCheckBox->isChecked())
            //drawAverageBars(screen, Qt::red);
			ui.screenLabel->setPixmap(screen);
		}
		else
		{
			ui.screenLabel->setPixmap(QPixmap());
			QString placeHolderStyle = tr("QLabel { background-color : black; }");
			ui.screenLabel->setStyleSheet(placeHolderStyle);
		}
	}
}

//Hardware status has changed
void AmbiLED::serialDeviceStatusChanged(bool status)
{
	if (status)
	{
		ui.deviceStatusLabel->setText(tr("Connected"));
		QIcon connectedIcon(connectedPixmap);
		ui.deviceStatusColourLabel->setIcon(connectedIcon);
		ui.deviceStatusColourLabel->setIconSize(connectedPixmap.rect().size());
		ui.deviceStatusColourLabel->setFixedSize(connectedPixmap.rect().size());
	}
	else
	{
		ui.deviceStatusLabel->setText(tr("Disconnected"));
		QIcon disconnectedIcon(disconnectedPixmap);
		ui.deviceStatusColourLabel->setIcon(disconnectedIcon);
		ui.deviceStatusColourLabel->setIconSize(disconnectedPixmap.rect().size());
		ui.deviceStatusColourLabel->setFixedSize(disconnectedPixmap.rect().size());
	}
}

//Send LED data
void AmbiLED::sendLEDData(QImage led)
{
	transformBrightness(led, pBrightness);
	if (this->isVisible())
	{
        ui.ledStrip->setPixmap(QPixmap::fromImage(led.scaled(ui.ledStrip->width(), ui.ledStrip->height(), Qt::IgnoreAspectRatio, Qt::FastTransformation)));
	}

	if (pSerialPortReady)
	{
		pSerialPortReady = false;
        qint64 elapsed = elapsedSerialTimer->elapsed();
		while (pSerialAverageQueue.count() < 10)
		{
			pSerialAverageQueue.enqueue(elapsed);
		}
		pSerialAverageQueue.enqueue(elapsed);
		pSerialAverageQueue.dequeue();
		//ui.serialLabel->setText(tr("Serial latency: %1 msec (%2/s)").arg(elapsed).arg(1000 / elapsed));
		elapsedSerialTimer->restart();
        emit writeLEDImage(led);
	}
}

void AmbiLED::updateUIElements()
{
	qreal averageSerialLatency = 0;
    for (int i = 0; i < pSerialAverageQueue.count(); ++i)
	{
		averageSerialLatency += pSerialAverageQueue.at(i);
	}
	averageSerialLatency /= pSerialAverageQueue.count();

	qreal averageFrameLatency = 0;
    for (int i = 0; i < pFrameAverageQueue.count(); ++i)
	{
		averageFrameLatency += pFrameAverageQueue.at(i);
	}
	averageFrameLatency /= pFrameAverageQueue.count();

	ui.serialLabel->setText(tr("Serial latency: %1 msec (%2 Hz)").arg(averageSerialLatency, 0, 'f', 2).arg(1000.0 / averageSerialLatency, 0, 'f', 2));
	ui.frameLabel->setText(tr("Latency: %1 msec (%2 Hz)").arg(averageFrameLatency, 0, 'f', 2).arg(1000.0 / averageFrameLatency, 0, 'f', 2));
}

void AmbiLED::showDeviceFirmware()
{
	if (!serialPort->connected())
		return;

    QMessageBox::information(this, tr("Device information"), tr("%1").arg(serialPort->getFirmwareVersion()), QMessageBox::Ok);
}

/*
void AmbiLED::transformBrightness(QImage &led, qreal brightness)
{
	if (brightness > 1.0 || brightness < 0.0)
		return;

	//Correct colour based on typical SMD5050 LED spectrum
	
    qreal rCorrection = brightness * static_cast<qreal>(((pColourCorrection >> 16) & 0xFF)) / 255.0;
    qreal gCorrection = brightness * static_cast<qreal>(((pColourCorrection >> 8) & 0xFF)) / 255.0;
    qreal bCorrection = brightness * static_cast<qreal>(((pColourCorrection) & 0xFF)) / 255.0;

    for (int i = 0; i < led.width(); ++i)
	{
		QColor pixel = led.pixelColor(i, 0);
		pixel.setRedF(pixel.redF() * rCorrection);
		pixel.setGreenF(pixel.greenF() * gCorrection);
		pixel.setBlueF(pixel.blueF() * bCorrection);
		led.setPixelColor(i, 0, pixel);
	}
}


QImage AmbiLED::generateTestPattern(const LEDPositions& positions, ImageStripsInput &outputLinePattern)
{
	//Generate a test pattern based on current positions
    int totalWidth = positions.getLength();
	QImage testPattern(totalWidth + 10, 1, QImage::Format_RGB32);

	/*
	Screen is shown from front

	LEDs are done clockwise from front

	//Positions as viewed from FRONT of monitor
	// -------- t ---------
	// |                  |
	// l                  r
	// |                  |
	// -- bl -- bg -- br --

	bl = 32
	br = 32
	l = 42
	r = 42
	top left = 37
	top right = 38

	Packing order is:

	bottom left (rtl) --> left (btt) --> top (ltr) --> right (ttb) --> bottom right (rtl)

	For QImage:

	rtl = reverse
	btt = reverse
	ltr = normal
	ttb = normal

	*/
/*
    int dataIndex = 0;
    int stripLength = 120;

    //Generate ImageStripsInput
	outputLinePattern.bottomLine = QImage(positions.getBottomCount(), 1, QImage::Format_RGB32);
	outputLinePattern.bottomLine.fill(Qt::black);
	outputLinePattern.topLine = QImage(positions.t, 1, QImage::Format_RGB32);
	outputLinePattern.leftLine = QImage(1, positions.l, QImage::Format_RGB32);
	outputLinePattern.rightLine = QImage(1, positions.r, QImage::Format_RGB32);

	//Start with first status led
	testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(64, 64, 64));

	//Start with bottom left
    for (int i = positions.bl - 1; i >= 0; --i)
	{
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(0, 255, 255));
		outputLinePattern.bottomLine.setPixel(QPoint(i, 0), qRgb(0, 255, 255));
	}

	//Append left
    for (int i = positions.l - 1; i >= 0; --i)
	{
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(0, 255, 0));
		outputLinePattern.leftLine.setPixel(QPoint(0, i), qRgb(0, 255, 0));
	}

	//Append left 50% of top
    int topBreak = positions.t / 2;
    for (int i = 0; i < topBreak; ++i)
	{
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(255, 255, 0));
		outputLinePattern.topLine.setPixel(QPoint(i, 0), qRgb(255, 255, 0));
	}

	//Append extra for additional end of strip
    for (int i = 0; i < stripLength - topBreak - positions.l - positions.bl - 1; ++i)
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(0, 0, 0));

	//Next status led
	testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(64, 64, 64));

	//Append bottom right
    for (int i = positions.getBottomCount() - positions.br; i < positions.getBottomCount(); ++i)
	{
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(255, 0, 255));
		outputLinePattern.bottomLine.setPixel(QPoint(i, 0), qRgb(255, 0, 255));
	}

	//Append right
    for (int i = positions.r - 1; i >= 0; --i)
	{
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(255, 0, 0));
		outputLinePattern.rightLine.setPixel(QPoint(0, i), qRgb(255, 0, 0));
	}

	//Append right 50% of top
    for (int i = positions.t - 1; i >= topBreak; --i)
	{
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(0, 0, 255));
		outputLinePattern.topLine.setPixel(QPoint(i, 0), qRgb(0, 0, 255));
	}

	//Append extra for additional end of strip
    for (int i = 0; i < 10; ++i)
		testPattern.setPixel(QPoint(dataIndex++, 0), qRgb(0, 0, 0));

	return testPattern;

}
*/

QPixmap& AmbiLED::drawArrow(QPixmap& input, ArrowDirection direction, Qt::GlobalColor colour, int offset, qreal height, qreal width, QString text)
{
	QPainter painter(&input);
    qreal inHeight = input.height();
    qreal inWidth = input.width();
	
    if (height < 0.0)
		height = inHeight;
    if (width < 0.0)
		width = inWidth;

    qreal requestedHeight = qMin(static_cast<qreal>(height), inHeight);
    qreal requestedWidth = qMin(static_cast<qreal>(width), inWidth);

	//Paint arrow body
	QPen pen;
	pen.setColor(Qt::black);
	QBrush brush(Qt::SolidPattern);
	brush.setColor(colour);

    qreal halfHeight = inHeight / 2.0;
    qreal halfWidth = inWidth / 2.0;
    qreal headHeight;
    qreal headWidth;
    qreal arrowWidth;

	QPolygonF arrow;

	pen.setWidth(1);
	painter.setPen(pen);
	painter.setBrush(brush);

	QTextOption textOption;
	textOption.setAlignment(Qt::AlignCenter);

	QFont font;
	font.setFamily("Verdana");
	font.setPointSizeF(6);
	painter.setFont(font);

	switch (direction)
	{
	case LeftArrow:
		/*
		(0,0)/|
		   /  -------------
		<                 |
		   \  -------------
		     \|

		Height is 50% of total image height
		Positive offset moves arrow to right
		*/

		headHeight = requestedHeight / 2.0;
        headWidth = qMin(requestedHeight / 2.0, inWidth / 2.0);
		arrowWidth = requestedHeight / 4.0;

		//Polygon for above
		arrow.append(QPointF(offset, halfHeight));
		arrow.append(QPointF(offset + headWidth, halfHeight - headHeight / 2.0));
		arrow.append(QPointF(offset + headWidth, halfHeight - arrowWidth / 2.0));
		arrow.append(QPointF(offset + requestedWidth, halfHeight - arrowWidth / 2.0));
		arrow.append(QPointF(offset + requestedWidth, halfHeight + arrowWidth / 2.0));
		arrow.append(QPointF(offset + headWidth, halfHeight + arrowWidth / 2.0));
		arrow.append(QPointF(offset + headWidth, halfHeight + headHeight / 2.0));
		
		painter.drawConvexPolygon(arrow);

		if (text != "")
		{
			//Draw text on top of the arrow
			painter.drawText(QRectF(offset, 0, requestedWidth, requestedHeight), text, textOption);
		}
			
		break;
	case RightArrow:
		/*
		(0,0)        |\
		 -------------  \
		|                 >
		 -------------  /
			         |/

		Height is 50% of total image height
		Positive offset moves arrow to left
		*/
		headHeight = requestedHeight / 2.0;
        headWidth = qMin(requestedHeight / 2.0, inWidth / 2.0);
		arrowWidth = requestedHeight / 4.0;

		//Polygon for above
		arrow.append(QPointF(inWidth - offset, halfHeight));
		arrow.append(QPointF(inWidth - headWidth - offset, halfHeight - headHeight / 2.0));
		arrow.append(QPointF(inWidth - headWidth - offset, halfHeight - arrowWidth / 2.0));
		arrow.append(QPointF(inWidth - requestedWidth - offset, halfHeight - arrowWidth / 2.0));
		arrow.append(QPointF(inWidth - requestedWidth - offset, halfHeight + arrowWidth / 2.0));
		arrow.append(QPointF(inWidth - headWidth - offset, halfHeight + arrowWidth / 2.0));
		arrow.append(QPointF(inWidth - headWidth - offset, halfHeight + headHeight / 2.0));

		painter.drawConvexPolygon(arrow);

		if (text != "")
		{
			//Draw text on top of the arrow
			painter.drawText(QRectF(inWidth - requestedWidth - offset, 0, requestedWidth, requestedHeight), text, textOption);
		}

		break;
	case UpArrow:
		/*
		(0,0) ^        
		    /   \
		  /_     _\
		    |   |
			|   |
			|   |
			-----

		Width is 50% of total image height
		Positive offset moves arrow down
		*/
        headHeight = qMin(requestedWidth / 2.0, inHeight / 2.0);
		headWidth = requestedWidth / 2.0;
		arrowWidth = requestedWidth / 4.0;

		//Polygon for above (clockwise starting at tip)
		arrow.append(QPointF(halfWidth, offset));
		arrow.append(QPointF(halfWidth + headWidth / 2.0, headHeight + offset));
		arrow.append(QPointF(halfWidth + arrowWidth / 2.0, headHeight + offset));
		arrow.append(QPointF(halfWidth + arrowWidth / 2.0, requestedHeight + offset));
		arrow.append(QPointF(halfWidth - arrowWidth / 2.0, requestedHeight + offset));
		arrow.append(QPointF(halfWidth - arrowWidth / 2.0, headHeight + offset));
		arrow.append(QPointF(halfWidth - headWidth / 2.0, headHeight + offset));

		painter.drawConvexPolygon(arrow);

		if (text != "")
		{
			painter.save();
			//Draw text on top of the arrow
			painter.translate(0, inHeight);
			painter.rotate(-90);
			
			painter.drawText(QRectF(inHeight - requestedHeight - offset, 0, requestedHeight, requestedWidth), text, textOption);
			painter.restore();
		}

		break;
	case DownArrow:
		/*
		(0,0)----
			|   |
		    |   |
			|   |
		   \-   -/
			\   /
			  *

		Width is 50% of total image height
		Positive offset moves arrow up
		*/
        headHeight = qMin(requestedWidth / 2.0, inHeight / 2.0);
		headWidth = requestedWidth / 2.0;
		arrowWidth = requestedWidth / 4.0;

		//Polygon for above (clockwise starting at tip)
		arrow.append(QPointF(halfWidth, inHeight - offset));
		arrow.append(QPointF(halfWidth + headWidth / 2.0, inHeight - headHeight - offset));
		arrow.append(QPointF(halfWidth + arrowWidth / 2.0, inHeight - headHeight - offset));
		arrow.append(QPointF(halfWidth + arrowWidth / 2.0, inHeight - requestedHeight - offset));
		arrow.append(QPointF(halfWidth - arrowWidth / 2.0, inHeight - requestedHeight - offset));
		arrow.append(QPointF(halfWidth - arrowWidth / 2.0, inHeight - headHeight - offset));
		arrow.append(QPointF(halfWidth - headWidth / 2.0, inHeight - headHeight - offset));

		painter.drawConvexPolygon(arrow);

		if (text != "")
		{
			painter.save();
			//Draw text on top of the arrow
			painter.translate(0, inHeight);
			painter.rotate(-90);

			painter.drawText(QRectF(offset, 0, requestedHeight, requestedWidth), text, textOption);
			painter.restore();
		}

		break;
	}
	
	return input;
}

QPixmap& AmbiLED::drawAverageBars(QPixmap& input, Qt::GlobalColor colour)
{
    int inHeight = input.height();
    int inWidth = input.width();
	
    int stripWidth = inHeight / 10; //Hardcoded 10%

	QPainter painter(&input);

	QPen pen;
	pen.setColor(colour);
	pen.setWidth(2);
	painter.setPen(pen);

	QBrush brush(Qt::NoBrush);
	brush.setColor(colour);
	painter.setBrush(brush);

	painter.drawRect(0, 0, inWidth, stripWidth);
	painter.drawRect(0, inHeight - stripWidth, inWidth, stripWidth);
	painter.drawRect(0, 0, stripWidth, inHeight);
	painter.drawRect(inWidth - stripWidth, 0, stripWidth, inHeight);

	return input;
}

void AmbiLED::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
	writeSettings();
	/*
    //pScreenUpdateTimer->stop();
	if (this->isVisible())
	{
		//QMessageBox::information(this, tr("AmbiLED Control"), tr("The program has been minimized to system tray"));
		tray->showMessage(tr("AmbiLED Control"), tr("AmbiLED Control has been minimised to system tray"), QSystemTrayIcon::Information, 1000);
		hide();
		event->ignore();
	}
	*/
}

void AmbiLED::hideEvent(QHideEvent* event)
{
	if (event->spontaneous())
	{
		if (!pNotificationShown)
		{
			tray->showMessage(tr("AmbiLED Control"), tr("AmbiLED Control has been minimised to system tray"), QSystemTrayIcon::Information, 500);
			pNotificationShown = true;
		}
		//event->ignore();
		hide();
	}
}

void AmbiLED::createActions()
{
	quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

	configureAction = new QAction(tr("&Configure"), this);
	connect(configureAction, SIGNAL(triggered()), this, SLOT(showNormal()));

	suspendAction = new QAction(tr("Capture &Screen"), this);
	suspendAction->setCheckable(true);
	suspendAction->setChecked(true);
	connect(suspendAction, SIGNAL(triggered()), this, SLOT(toggleCapture()));
}

void AmbiLED::createTrayIcon()
{
	QMenu *trayMenu = new QMenu(this);
	trayMenu->addAction(suspendAction);
	trayMenu->addAction(configureAction);
	trayMenu->addAction(quitAction);	

	tray = new QSystemTrayIcon(this);
	tray->setToolTip(tr("AmbiLED v%1").arg(VERSION_STR));
	tray->setIcon(QIcon(":/AmbiLED/monitor_icon"));
	tray->setContextMenu(trayMenu);

	connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayClicked(QSystemTrayIcon::ActivationReason)));
}

void AmbiLED::createPixmaps()
{
	//Create transparent surfaces
	connectedPixmap = QPixmap(16,16);
	connectedPixmap.fill(Qt::transparent);
	disconnectedPixmap = QPixmap(16, 16);
	disconnectedPixmap.fill(Qt::transparent);

	QPen pen;
	pen.setColor(Qt::black);
	pen.setWidth(1);

	QBrush brush;
	brush.setStyle(Qt::SolidPattern);

	//Create connected icon (green circle)
	brush.setColor(Qt::green);
	QPainter connectedPainter(&connectedPixmap);
	connectedPainter.setPen(pen);
	connectedPainter.setBrush(brush);

	connectedPainter.drawEllipse(0, 0, 15, 15);
	
	//Create disconnected icon (red circle)
	brush.setColor(Qt::red);
	QPainter disconnectedPainter(&disconnectedPixmap);
	disconnectedPainter.setPen(pen);
	disconnectedPainter.setBrush(brush);

	disconnectedPainter.drawEllipse(0, 0, 15, 15);
}
