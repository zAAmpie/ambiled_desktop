/********************************************************************************
** Form generated from reading UI file 'ambiled.ui'
**
** Created by: Qt User Interface Compiler version 6.5.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AMBILED_H
#define UI_AMBILED_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AmbiLEDClass
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout_4;
    QLabel *ledStrip;
    QLabel *serialLabel;
    QHBoxLayout *horizontalLayout_17;
    QLabel *debugLabel;
    QSpacerItem *horizontalSpacer_9;
    QLabel *deviceStatusLabel;
    QPushButton *deviceStatusColourLabel;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_16;
    QLabel *brightnessLabel;
    QSlider *brightnessSlider;
    QLabel *brightnessValueLabel;
    QFormLayout *formLayout_2;
    QLabel *refreshRateLabel;
    QComboBox *refreshRateComboBox;
    QFormLayout *formLayout;
    QLabel *comPortLabel;
    QComboBox *comPortComboBox;
    QHBoxLayout *horizontalLayout_7;
    QLabel *frameLabel;
    QSpacerItem *horizontalSpacer_7;
    QCheckBox *captureCheckBox;
    QCheckBox *visualiseCheckBox;
    QTabWidget *tabWidget;
    QWidget *displayTab;
    QVBoxLayout *verticalLayout_6;
    QHBoxLayout *horizontalLayout_6;
    QVBoxLayout *verticalLayout_3;
    QSpacerItem *verticalSpacer_2;
    QSpinBox *leftLEDCount;
    QSpacerItem *verticalSpacer;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_2;
    QSpinBox *topLEDCount;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout;
    QLabel *TLlabel;
    QLabel *topImage;
    QLabel *TRlabel;
    QHBoxLayout *horizontalLayout_2;
    QLabel *leftImage;
    QLabel *screenLabel;
    QLabel *rightImage;
    QHBoxLayout *horizontalLayout_3;
    QLabel *BLlabel;
    QLabel *bottomImage;
    QLabel *BRlabel;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_5;
    QSpinBox *bottomLeftLEDCount;
    QSpacerItem *horizontalSpacer_4;
    QSpinBox *gapLEDCount;
    QSpacerItem *horizontalSpacer_3;
    QSpinBox *bottomRightLEDCount;
    QSpacerItem *horizontalSpacer_6;
    QVBoxLayout *verticalLayout_4;
    QSpacerItem *verticalSpacer_4;
    QSpinBox *rightLEDCount;
    QSpacerItem *verticalSpacer_3;
    QWidget *advancedTab;
    QGridLayout *gridLayout_2;
    QComboBox *captureComboBox;
    QLabel *colourCorrectionLabel;
    QHBoxLayout *horizontalLayout_9;
    QSpacerItem *horizontalSpacer_8;
    QPushButton *discardConfigButton;
    QPushButton *saveConfigButton;
    QLabel *label_5;
    QSpacerItem *verticalSpacer_5;
    QLabel *label_7;
    QComboBox *averageMethodComboBox;
    QLabel *captureLabel;
    QFormLayout *formLayout_4;
    QLabel *label_4;
    QComboBox *colourTemperatureComboBox;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout_11;
    QSlider *redColourSlider;
    QLabel *redColourLabel;
    QHBoxLayout *horizontalLayout_13;
    QSlider *greenColourSlider;
    QLabel *greenColourLabel;
    QHBoxLayout *horizontalLayout_14;
    QSlider *blueColourSlider;
    QLabel *blueColourLabel;
    QComboBox *highFrequencyFilterComboBox;
    QLabel *label_6;
    QCheckBox *debugCheckBox;
    QListWidget *processingList;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *AmbiLEDClass)
    {
        if (AmbiLEDClass->objectName().isEmpty())
            AmbiLEDClass->setObjectName("AmbiLEDClass");
        AmbiLEDClass->resize(732, 722);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(AmbiLEDClass->sizePolicy().hasHeightForWidth());
        AmbiLEDClass->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/AmbiLED/monitor_icon"), QSize(), QIcon::Normal, QIcon::Off);
        AmbiLEDClass->setWindowIcon(icon);
        centralWidget = new QWidget(AmbiLEDClass);
        centralWidget->setObjectName("centralWidget");
        gridLayout_4 = new QGridLayout(centralWidget);
        gridLayout_4->setSpacing(6);
        gridLayout_4->setContentsMargins(11, 11, 11, 11);
        gridLayout_4->setObjectName("gridLayout_4");
        ledStrip = new QLabel(centralWidget);
        ledStrip->setObjectName("ledStrip");
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(ledStrip->sizePolicy().hasHeightForWidth());
        ledStrip->setSizePolicy(sizePolicy1);
        ledStrip->setMinimumSize(QSize(0, 20));
        ledStrip->setMaximumSize(QSize(16777215, 20));

        gridLayout_4->addWidget(ledStrip, 3, 0, 1, 1);

        serialLabel = new QLabel(centralWidget);
        serialLabel->setObjectName("serialLabel");

        gridLayout_4->addWidget(serialLabel, 4, 0, 1, 1);

        horizontalLayout_17 = new QHBoxLayout();
        horizontalLayout_17->setSpacing(6);
        horizontalLayout_17->setObjectName("horizontalLayout_17");
        debugLabel = new QLabel(centralWidget);
        debugLabel->setObjectName("debugLabel");

        horizontalLayout_17->addWidget(debugLabel);

        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_17->addItem(horizontalSpacer_9);

        deviceStatusLabel = new QLabel(centralWidget);
        deviceStatusLabel->setObjectName("deviceStatusLabel");

        horizontalLayout_17->addWidget(deviceStatusLabel);

        deviceStatusColourLabel = new QPushButton(centralWidget);
        deviceStatusColourLabel->setObjectName("deviceStatusColourLabel");
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(deviceStatusColourLabel->sizePolicy().hasHeightForWidth());
        deviceStatusColourLabel->setSizePolicy(sizePolicy2);
        deviceStatusColourLabel->setMinimumSize(QSize(16, 16));
        deviceStatusColourLabel->setMaximumSize(QSize(16, 16));
        deviceStatusColourLabel->setFlat(true);

        horizontalLayout_17->addWidget(deviceStatusColourLabel);


        gridLayout_4->addLayout(horizontalLayout_17, 5, 0, 1, 1);

        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName("gridLayout");
        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setSpacing(6);
        horizontalLayout_16->setObjectName("horizontalLayout_16");
        brightnessLabel = new QLabel(centralWidget);
        brightnessLabel->setObjectName("brightnessLabel");

        horizontalLayout_16->addWidget(brightnessLabel);

        brightnessSlider = new QSlider(centralWidget);
        brightnessSlider->setObjectName("brightnessSlider");
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(brightnessSlider->sizePolicy().hasHeightForWidth());
        brightnessSlider->setSizePolicy(sizePolicy3);
        brightnessSlider->setMaximum(100);
        brightnessSlider->setValue(15);
        brightnessSlider->setOrientation(Qt::Horizontal);

        horizontalLayout_16->addWidget(brightnessSlider);

        brightnessValueLabel = new QLabel(centralWidget);
        brightnessValueLabel->setObjectName("brightnessValueLabel");
        sizePolicy2.setHeightForWidth(brightnessValueLabel->sizePolicy().hasHeightForWidth());
        brightnessValueLabel->setSizePolicy(sizePolicy2);
        brightnessValueLabel->setMinimumSize(QSize(29, 0));

        horizontalLayout_16->addWidget(brightnessValueLabel);


        gridLayout->addLayout(horizontalLayout_16, 0, 2, 1, 1);

        formLayout_2 = new QFormLayout();
        formLayout_2->setSpacing(6);
        formLayout_2->setObjectName("formLayout_2");
        formLayout_2->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        refreshRateLabel = new QLabel(centralWidget);
        refreshRateLabel->setObjectName("refreshRateLabel");

        formLayout_2->setWidget(0, QFormLayout::LabelRole, refreshRateLabel);

        refreshRateComboBox = new QComboBox(centralWidget);
        refreshRateComboBox->setObjectName("refreshRateComboBox");
        sizePolicy2.setHeightForWidth(refreshRateComboBox->sizePolicy().hasHeightForWidth());
        refreshRateComboBox->setSizePolicy(sizePolicy2);

        formLayout_2->setWidget(0, QFormLayout::FieldRole, refreshRateComboBox);


        gridLayout->addLayout(formLayout_2, 0, 1, 1, 1);

        formLayout = new QFormLayout();
        formLayout->setSpacing(6);
        formLayout->setObjectName("formLayout");
        comPortLabel = new QLabel(centralWidget);
        comPortLabel->setObjectName("comPortLabel");

        formLayout->setWidget(0, QFormLayout::LabelRole, comPortLabel);

        comPortComboBox = new QComboBox(centralWidget);
        comPortComboBox->setObjectName("comPortComboBox");
        sizePolicy2.setHeightForWidth(comPortComboBox->sizePolicy().hasHeightForWidth());
        comPortComboBox->setSizePolicy(sizePolicy2);
        comPortComboBox->setEditable(true);

        formLayout->setWidget(0, QFormLayout::FieldRole, comPortComboBox);


        gridLayout->addLayout(formLayout, 0, 0, 1, 1);


        gridLayout_4->addLayout(gridLayout, 0, 0, 1, 1);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        frameLabel = new QLabel(centralWidget);
        frameLabel->setObjectName("frameLabel");

        horizontalLayout_7->addWidget(frameLabel);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_7);

        captureCheckBox = new QCheckBox(centralWidget);
        captureCheckBox->setObjectName("captureCheckBox");

        horizontalLayout_7->addWidget(captureCheckBox);

        visualiseCheckBox = new QCheckBox(centralWidget);
        visualiseCheckBox->setObjectName("visualiseCheckBox");
        QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(visualiseCheckBox->sizePolicy().hasHeightForWidth());
        visualiseCheckBox->setSizePolicy(sizePolicy4);
        visualiseCheckBox->setChecked(false);

        horizontalLayout_7->addWidget(visualiseCheckBox);


        gridLayout_4->addLayout(horizontalLayout_7, 1, 0, 1, 1);

        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName("tabWidget");
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy5);
        displayTab = new QWidget();
        displayTab->setObjectName("displayTab");
        QSizePolicy sizePolicy6(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(displayTab->sizePolicy().hasHeightForWidth());
        displayTab->setSizePolicy(sizePolicy6);
        displayTab->setAutoFillBackground(false);
        verticalLayout_6 = new QVBoxLayout(displayTab);
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setContentsMargins(11, 11, 11, 11);
        verticalLayout_6->setObjectName("verticalLayout_6");
        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_2);

        leftLEDCount = new QSpinBox(displayTab);
        leftLEDCount->setObjectName("leftLEDCount");
        leftLEDCount->setMinimum(10);
        leftLEDCount->setMaximum(100);
        leftLEDCount->setValue(42);

        verticalLayout_3->addWidget(leftLEDCount);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);


        horizontalLayout_6->addLayout(verticalLayout_3);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);

        topLEDCount = new QSpinBox(displayTab);
        topLEDCount->setObjectName("topLEDCount");
        topLEDCount->setMinimum(10);
        topLEDCount->setMaximum(100);
        topLEDCount->setValue(75);

        horizontalLayout_4->addWidget(topLEDCount);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(horizontalLayout_4);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName("horizontalLayout");
        TLlabel = new QLabel(displayTab);
        TLlabel->setObjectName("TLlabel");
        sizePolicy4.setHeightForWidth(TLlabel->sizePolicy().hasHeightForWidth());
        TLlabel->setSizePolicy(sizePolicy4);
        TLlabel->setMinimumSize(QSize(50, 50));
        TLlabel->setMaximumSize(QSize(50, 50));

        horizontalLayout->addWidget(TLlabel);

        topImage = new QLabel(displayTab);
        topImage->setObjectName("topImage");
        QSizePolicy sizePolicy7(QSizePolicy::Ignored, QSizePolicy::Fixed);
        sizePolicy7.setHorizontalStretch(0);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(topImage->sizePolicy().hasHeightForWidth());
        topImage->setSizePolicy(sizePolicy7);
        topImage->setMinimumSize(QSize(0, 50));
        topImage->setMaximumSize(QSize(16777215, 50));

        horizontalLayout->addWidget(topImage);

        TRlabel = new QLabel(displayTab);
        TRlabel->setObjectName("TRlabel");
        sizePolicy4.setHeightForWidth(TRlabel->sizePolicy().hasHeightForWidth());
        TRlabel->setSizePolicy(sizePolicy4);
        TRlabel->setMinimumSize(QSize(50, 50));
        TRlabel->setMaximumSize(QSize(50, 50));

        horizontalLayout->addWidget(TRlabel);


        verticalLayout_2->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        leftImage = new QLabel(displayTab);
        leftImage->setObjectName("leftImage");
        QSizePolicy sizePolicy8(QSizePolicy::Fixed, QSizePolicy::Ignored);
        sizePolicy8.setHorizontalStretch(0);
        sizePolicy8.setVerticalStretch(0);
        sizePolicy8.setHeightForWidth(leftImage->sizePolicy().hasHeightForWidth());
        leftImage->setSizePolicy(sizePolicy8);
        leftImage->setMinimumSize(QSize(50, 0));
        leftImage->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_2->addWidget(leftImage);

        screenLabel = new QLabel(displayTab);
        screenLabel->setObjectName("screenLabel");
        QSizePolicy sizePolicy9(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy9.setHorizontalStretch(1);
        sizePolicy9.setVerticalStretch(1);
        sizePolicy9.setHeightForWidth(screenLabel->sizePolicy().hasHeightForWidth());
        screenLabel->setSizePolicy(sizePolicy9);
        screenLabel->setMinimumSize(QSize(0, 0));
        screenLabel->setAutoFillBackground(false);
        screenLabel->setFrameShape(QFrame::Box);
        screenLabel->setLineWidth(2);
        screenLabel->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(screenLabel);

        rightImage = new QLabel(displayTab);
        rightImage->setObjectName("rightImage");
        sizePolicy8.setHeightForWidth(rightImage->sizePolicy().hasHeightForWidth());
        rightImage->setSizePolicy(sizePolicy8);
        rightImage->setMinimumSize(QSize(50, 0));
        rightImage->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_2->addWidget(rightImage);


        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        BLlabel = new QLabel(displayTab);
        BLlabel->setObjectName("BLlabel");
        sizePolicy4.setHeightForWidth(BLlabel->sizePolicy().hasHeightForWidth());
        BLlabel->setSizePolicy(sizePolicy4);
        BLlabel->setMinimumSize(QSize(50, 50));
        BLlabel->setMaximumSize(QSize(50, 50));

        horizontalLayout_3->addWidget(BLlabel);

        bottomImage = new QLabel(displayTab);
        bottomImage->setObjectName("bottomImage");
        sizePolicy7.setHeightForWidth(bottomImage->sizePolicy().hasHeightForWidth());
        bottomImage->setSizePolicy(sizePolicy7);
        bottomImage->setMinimumSize(QSize(0, 50));
        bottomImage->setMaximumSize(QSize(16777215, 50));

        horizontalLayout_3->addWidget(bottomImage);

        BRlabel = new QLabel(displayTab);
        BRlabel->setObjectName("BRlabel");
        sizePolicy4.setHeightForWidth(BRlabel->sizePolicy().hasHeightForWidth());
        BRlabel->setSizePolicy(sizePolicy4);
        BRlabel->setMinimumSize(QSize(50, 50));
        BRlabel->setMaximumSize(QSize(50, 50));

        horizontalLayout_3->addWidget(BRlabel);


        verticalLayout_2->addLayout(horizontalLayout_3);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_5);

        bottomLeftLEDCount = new QSpinBox(displayTab);
        bottomLeftLEDCount->setObjectName("bottomLeftLEDCount");
        bottomLeftLEDCount->setMinimum(5);
        bottomLeftLEDCount->setMaximum(100);
        bottomLeftLEDCount->setValue(32);

        horizontalLayout_5->addWidget(bottomLeftLEDCount);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_4);

        gapLEDCount = new QSpinBox(displayTab);
        gapLEDCount->setObjectName("gapLEDCount");
        gapLEDCount->setMaximum(100);
        gapLEDCount->setValue(12);

        horizontalLayout_5->addWidget(gapLEDCount);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_3);

        bottomRightLEDCount = new QSpinBox(displayTab);
        bottomRightLEDCount->setObjectName("bottomRightLEDCount");
        bottomRightLEDCount->setMinimum(5);
        bottomRightLEDCount->setMaximum(100);
        bottomRightLEDCount->setValue(32);

        horizontalLayout_5->addWidget(bottomRightLEDCount);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_6);


        verticalLayout_2->addLayout(horizontalLayout_5);

        verticalLayout_2->setStretch(2, 1);

        horizontalLayout_6->addLayout(verticalLayout_2);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(6);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_4);

        rightLEDCount = new QSpinBox(displayTab);
        rightLEDCount->setObjectName("rightLEDCount");
        rightLEDCount->setMinimum(10);
        rightLEDCount->setMaximum(100);
        rightLEDCount->setValue(42);

        verticalLayout_4->addWidget(rightLEDCount);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_3);


        horizontalLayout_6->addLayout(verticalLayout_4);


        verticalLayout_6->addLayout(horizontalLayout_6);

        tabWidget->addTab(displayTab, QString());
        advancedTab = new QWidget();
        advancedTab->setObjectName("advancedTab");
        gridLayout_2 = new QGridLayout(advancedTab);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName("gridLayout_2");
        captureComboBox = new QComboBox(advancedTab);
        captureComboBox->setObjectName("captureComboBox");

        gridLayout_2->addWidget(captureComboBox, 0, 2, 1, 1);

        colourCorrectionLabel = new QLabel(advancedTab);
        colourCorrectionLabel->setObjectName("colourCorrectionLabel");

        gridLayout_2->addWidget(colourCorrectionLabel, 1, 0, 1, 1);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_8);

        discardConfigButton = new QPushButton(advancedTab);
        discardConfigButton->setObjectName("discardConfigButton");

        horizontalLayout_9->addWidget(discardConfigButton);

        saveConfigButton = new QPushButton(advancedTab);
        saveConfigButton->setObjectName("saveConfigButton");

        horizontalLayout_9->addWidget(saveConfigButton);


        gridLayout_2->addLayout(horizontalLayout_9, 8, 2, 1, 1);

        label_5 = new QLabel(advancedTab);
        label_5->setObjectName("label_5");

        gridLayout_2->addWidget(label_5, 2, 0, 1, 1);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer_5, 7, 0, 1, 1);

        label_7 = new QLabel(advancedTab);
        label_7->setObjectName("label_7");

        gridLayout_2->addWidget(label_7, 4, 0, 1, 1);

        averageMethodComboBox = new QComboBox(advancedTab);
        averageMethodComboBox->setObjectName("averageMethodComboBox");

        gridLayout_2->addWidget(averageMethodComboBox, 2, 2, 1, 1);

        captureLabel = new QLabel(advancedTab);
        captureLabel->setObjectName("captureLabel");
        QSizePolicy sizePolicy10(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy10.setHorizontalStretch(0);
        sizePolicy10.setVerticalStretch(0);
        sizePolicy10.setHeightForWidth(captureLabel->sizePolicy().hasHeightForWidth());
        captureLabel->setSizePolicy(sizePolicy10);

        gridLayout_2->addWidget(captureLabel, 0, 0, 1, 1);

        formLayout_4 = new QFormLayout();
        formLayout_4->setSpacing(6);
        formLayout_4->setObjectName("formLayout_4");
        label_4 = new QLabel(advancedTab);
        label_4->setObjectName("label_4");

        formLayout_4->setWidget(0, QFormLayout::LabelRole, label_4);

        colourTemperatureComboBox = new QComboBox(advancedTab);
        colourTemperatureComboBox->setObjectName("colourTemperatureComboBox");

        formLayout_4->setWidget(0, QFormLayout::FieldRole, colourTemperatureComboBox);

        label = new QLabel(advancedTab);
        label->setObjectName("label");

        formLayout_4->setWidget(1, QFormLayout::LabelRole, label);

        label_2 = new QLabel(advancedTab);
        label_2->setObjectName("label_2");

        formLayout_4->setWidget(2, QFormLayout::LabelRole, label_2);

        label_3 = new QLabel(advancedTab);
        label_3->setObjectName("label_3");

        formLayout_4->setWidget(3, QFormLayout::LabelRole, label_3);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setSpacing(6);
        horizontalLayout_11->setObjectName("horizontalLayout_11");
        redColourSlider = new QSlider(advancedTab);
        redColourSlider->setObjectName("redColourSlider");
        redColourSlider->setEnabled(false);
        redColourSlider->setMaximum(255);
        redColourSlider->setValue(255);
        redColourSlider->setOrientation(Qt::Horizontal);

        horizontalLayout_11->addWidget(redColourSlider);

        redColourLabel = new QLabel(advancedTab);
        redColourLabel->setObjectName("redColourLabel");
        sizePolicy10.setHeightForWidth(redColourLabel->sizePolicy().hasHeightForWidth());
        redColourLabel->setSizePolicy(sizePolicy10);

        horizontalLayout_11->addWidget(redColourLabel);


        formLayout_4->setLayout(1, QFormLayout::FieldRole, horizontalLayout_11);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setSpacing(6);
        horizontalLayout_13->setObjectName("horizontalLayout_13");
        greenColourSlider = new QSlider(advancedTab);
        greenColourSlider->setObjectName("greenColourSlider");
        greenColourSlider->setEnabled(false);
        greenColourSlider->setMaximum(255);
        greenColourSlider->setValue(255);
        greenColourSlider->setOrientation(Qt::Horizontal);

        horizontalLayout_13->addWidget(greenColourSlider);

        greenColourLabel = new QLabel(advancedTab);
        greenColourLabel->setObjectName("greenColourLabel");
        sizePolicy10.setHeightForWidth(greenColourLabel->sizePolicy().hasHeightForWidth());
        greenColourLabel->setSizePolicy(sizePolicy10);

        horizontalLayout_13->addWidget(greenColourLabel);


        formLayout_4->setLayout(2, QFormLayout::FieldRole, horizontalLayout_13);

        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setSpacing(6);
        horizontalLayout_14->setObjectName("horizontalLayout_14");
        blueColourSlider = new QSlider(advancedTab);
        blueColourSlider->setObjectName("blueColourSlider");
        blueColourSlider->setEnabled(false);
        blueColourSlider->setMaximum(255);
        blueColourSlider->setValue(255);
        blueColourSlider->setOrientation(Qt::Horizontal);

        horizontalLayout_14->addWidget(blueColourSlider);

        blueColourLabel = new QLabel(advancedTab);
        blueColourLabel->setObjectName("blueColourLabel");
        sizePolicy10.setHeightForWidth(blueColourLabel->sizePolicy().hasHeightForWidth());
        blueColourLabel->setSizePolicy(sizePolicy10);

        horizontalLayout_14->addWidget(blueColourLabel);


        formLayout_4->setLayout(3, QFormLayout::FieldRole, horizontalLayout_14);


        gridLayout_2->addLayout(formLayout_4, 1, 2, 1, 1);

        highFrequencyFilterComboBox = new QComboBox(advancedTab);
        highFrequencyFilterComboBox->setObjectName("highFrequencyFilterComboBox");

        gridLayout_2->addWidget(highFrequencyFilterComboBox, 3, 2, 1, 1);

        label_6 = new QLabel(advancedTab);
        label_6->setObjectName("label_6");

        gridLayout_2->addWidget(label_6, 3, 0, 1, 1);

        debugCheckBox = new QCheckBox(advancedTab);
        debugCheckBox->setObjectName("debugCheckBox");
        sizePolicy4.setHeightForWidth(debugCheckBox->sizePolicy().hasHeightForWidth());
        debugCheckBox->setSizePolicy(sizePolicy4);

        gridLayout_2->addWidget(debugCheckBox, 6, 0, 1, 1);

        processingList = new QListWidget(advancedTab);
        processingList->setObjectName("processingList");
        sizePolicy5.setHeightForWidth(processingList->sizePolicy().hasHeightForWidth());
        processingList->setSizePolicy(sizePolicy5);
        processingList->setMaximumSize(QSize(16777215, 60));
        processingList->setSelectionMode(QAbstractItemView::MultiSelection);
        processingList->setWordWrap(false);

        gridLayout_2->addWidget(processingList, 4, 2, 1, 1);

        tabWidget->addTab(advancedTab, QString());

        gridLayout_4->addWidget(tabWidget, 2, 0, 1, 1);

        AmbiLEDClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(AmbiLEDClass);
        statusBar->setObjectName("statusBar");
        AmbiLEDClass->setStatusBar(statusBar);

        retranslateUi(AmbiLEDClass);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(AmbiLEDClass);
    } // setupUi

    void retranslateUi(QMainWindow *AmbiLEDClass)
    {
        AmbiLEDClass->setWindowTitle(QCoreApplication::translate("AmbiLEDClass", "AmbiLED Control", nullptr));
        ledStrip->setText(QCoreApplication::translate("AmbiLEDClass", "led", nullptr));
        serialLabel->setText(QCoreApplication::translate("AmbiLEDClass", "seriallabel", nullptr));
        debugLabel->setText(QCoreApplication::translate("AmbiLEDClass", "debug", nullptr));
        deviceStatusLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Disconnected", nullptr));
        deviceStatusColourLabel->setText(QString());
        brightnessLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Brightness", nullptr));
        brightnessValueLabel->setText(QCoreApplication::translate("AmbiLEDClass", "100%", nullptr));
        refreshRateLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Refresh rate", nullptr));
        comPortLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Communication port", nullptr));
        frameLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Frame counter", nullptr));
        captureCheckBox->setText(QCoreApplication::translate("AmbiLEDClass", "Capture", nullptr));
        visualiseCheckBox->setText(QCoreApplication::translate("AmbiLEDClass", "Visualise", nullptr));
        topLEDCount->setSpecialValueText(QString());
        topLEDCount->setSuffix(QString());
        topLEDCount->setPrefix(QString());
        TLlabel->setText(QString());
        topImage->setText(QCoreApplication::translate("AmbiLEDClass", "top", nullptr));
        TRlabel->setText(QString());
        leftImage->setText(QCoreApplication::translate("AmbiLEDClass", "left", nullptr));
        screenLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Screen", nullptr));
        rightImage->setText(QCoreApplication::translate("AmbiLEDClass", "right", nullptr));
        BLlabel->setText(QString());
        bottomImage->setText(QCoreApplication::translate("AmbiLEDClass", "bottom", nullptr));
        BRlabel->setText(QString());
        gapLEDCount->setSuffix(QString());
        tabWidget->setTabText(tabWidget->indexOf(displayTab), QCoreApplication::translate("AmbiLEDClass", "Display", nullptr));
        colourCorrectionLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Colour correction", nullptr));
        discardConfigButton->setText(QCoreApplication::translate("AmbiLEDClass", "&Discard", nullptr));
        saveConfigButton->setText(QCoreApplication::translate("AmbiLEDClass", "&Save", nullptr));
        label_5->setText(QCoreApplication::translate("AmbiLEDClass", "Averaging method", nullptr));
        label_7->setText(QCoreApplication::translate("AmbiLEDClass", "Processing", nullptr));
        captureLabel->setText(QCoreApplication::translate("AmbiLEDClass", "Capture method", nullptr));
        label_4->setText(QCoreApplication::translate("AmbiLEDClass", "Preset", nullptr));
        label->setText(QCoreApplication::translate("AmbiLEDClass", "Red", nullptr));
        label_2->setText(QCoreApplication::translate("AmbiLEDClass", "Green", nullptr));
        label_3->setText(QCoreApplication::translate("AmbiLEDClass", "Blue", nullptr));
        redColourLabel->setText(QCoreApplication::translate("AmbiLEDClass", "255", nullptr));
        greenColourLabel->setText(QCoreApplication::translate("AmbiLEDClass", "255", nullptr));
        blueColourLabel->setText(QCoreApplication::translate("AmbiLEDClass", "255", nullptr));
        label_6->setText(QCoreApplication::translate("AmbiLEDClass", "High frequency filter", nullptr));
        debugCheckBox->setText(QCoreApplication::translate("AmbiLEDClass", "Debug", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(advancedTab), QCoreApplication::translate("AmbiLEDClass", "Advanced", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AmbiLEDClass: public Ui_AmbiLEDClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AMBILED_H
