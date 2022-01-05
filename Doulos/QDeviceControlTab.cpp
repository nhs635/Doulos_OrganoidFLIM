
#include "QDeviceControlTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Dialog/FlimCalibDlg.h>
#include <Doulos/Viewer/QImageView.h>

#include <DeviceControl/FLImControl/PmtGainControl.h>
#include <DeviceControl/FLImControl/FlimTrigger.h>
#include <DeviceControl/IPGPhotonicsLaser/IPGPhotonicsLaser.h>
#include <DeviceControl/ResonantScan/ResonantScan.h>
#include <DeviceControl/GalvoScan/GalvoScan.h>
#include <DeviceControl/ZaberStage/ZaberStage.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <Common/Array.h>

#include <iostream>
#include <thread>


QDeviceControlTab::QDeviceControlTab(QWidget *parent) :
    QDialog(parent), m_pPmtGainControl(nullptr), m_pFlimTrigControl(nullptr),
    m_pIPGPhotonicsLaser(nullptr), m_pFlimCalibDlg(nullptr), 
	m_pResonantScan(nullptr), m_pGalvoScan(nullptr), m_pZaberStage(nullptr)
{
	// Set main window objects
    m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(0);

	m_pVBoxLayout_FlimControl = new QVBoxLayout;
	m_pVBoxLayout_FlimControl->setSpacing(3);
    m_pGroupBox_FlimControl = new QGroupBox;

    createPmtGainControl();
    createFlimTriggeringControl();
	createFlimLaserControl();
    createFlimCalibControl();
    createScanControl();
	createZaberStageControl();
	
    // Set layout
    setLayout(m_pVBoxLayout);
}

QDeviceControlTab::~QDeviceControlTab()
{
    if (m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(false);
    if (m_pCheckBox_FlimLaserTrigControl->isChecked()) m_pCheckBox_FlimLaserTrigControl->setChecked(false);
    if (m_pCheckBox_FlimLaserControl->isChecked()) m_pCheckBox_FlimLaserControl->setChecked(false);
	if (m_pCheckBox_ResonantScanControl->isChecked()) m_pCheckBox_ResonantScanControl->setChecked(false);
	if (m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(false);
	if (m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(false);
}

void QDeviceControlTab::closeEvent(QCloseEvent* e)
{
    e->accept();
}

void QDeviceControlTab::setControlsStatus(bool enabled)
{
	if (!enabled)
	{
		if (m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(false);
		if (m_pCheckBox_FlimLaserTrigControl->isChecked()) m_pCheckBox_FlimLaserTrigControl->setChecked(false);
		if (m_pCheckBox_FlimLaserControl->isChecked()) m_pCheckBox_FlimLaserControl->setChecked(false);
		if (m_pCheckBox_ResonantScanControl->isChecked()) m_pCheckBox_ResonantScanControl->setChecked(false);
		if (m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(false);
		if (m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(false);
	}
	else
	{
		if (!m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(true);
		if (!m_pCheckBox_FlimLaserTrigControl->isChecked()) m_pCheckBox_FlimLaserTrigControl->setChecked(true);
		if (!m_pCheckBox_FlimLaserControl->isChecked()) m_pCheckBox_FlimLaserControl->setChecked(true);
		if (m_pCheckBox_ResonantScanControl->isChecked()) m_pCheckBox_ResonantScanControl->setChecked(true);
		if (!m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(true);
		if (!m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(true);
	}
}

void QDeviceControlTab::createPmtGainControl()
{
    // Create widgets for PMT gain control
    QHBoxLayout *pHBoxLayout_PmtGainControl = new QHBoxLayout;
    pHBoxLayout_PmtGainControl->setSpacing(3);

    m_pCheckBox_PmtGainControl = new QCheckBox(m_pGroupBox_FlimControl);
    m_pCheckBox_PmtGainControl->setText("Apply PMT Gain Voltage");
    m_pCheckBox_PmtGainControl->setFixedWidth(200);

    m_pLineEdit_PmtGainVoltage = new QLineEdit(m_pGroupBox_FlimControl);
    m_pLineEdit_PmtGainVoltage->setFixedWidth(41);
    m_pLineEdit_PmtGainVoltage->setText(QString::number(m_pConfig->pmtGainVoltage, 'f', 3));
    m_pLineEdit_PmtGainVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_PmtGainVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pLineEdit_PmtGainVoltage->setEnabled(true);

    m_pLabel_PmtGainVoltage = new QLabel("V", m_pGroupBox_FlimControl);
    m_pLabel_PmtGainVoltage->setBuddy(m_pLineEdit_PmtGainVoltage);
    m_pLabel_PmtGainVoltage->setEnabled(true);

    pHBoxLayout_PmtGainControl->addWidget(m_pCheckBox_PmtGainControl);
    pHBoxLayout_PmtGainControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_PmtGainControl->addWidget(m_pLineEdit_PmtGainVoltage);
    pHBoxLayout_PmtGainControl->addWidget(m_pLabel_PmtGainVoltage);

    m_pVBoxLayout_FlimControl->addItem(pHBoxLayout_PmtGainControl);

    // Connect signal and slot
    connect(m_pCheckBox_PmtGainControl, SIGNAL(toggled(bool)), this, SLOT(applyPmtGainVoltage(bool)));
    connect(m_pLineEdit_PmtGainVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changePmtGainVoltage(const QString &)));
}

void QDeviceControlTab::createFlimTriggeringControl()
{
    // Create widgets for FLIM laser control
    QHBoxLayout *pHBoxLayout_FlimLaserTrigControl = new QHBoxLayout;
    pHBoxLayout_FlimLaserTrigControl->setSpacing(3);

    m_pCheckBox_FlimLaserTrigControl = new QCheckBox(m_pGroupBox_FlimControl);
    m_pCheckBox_FlimLaserTrigControl->setText("Start FLIm Laser Triggering");
	
	QLocale locale(QLocale::English);
    m_pLabel_RepetitionRate = new QLabel(locale.toString(FLIM_LASER_REP_RATE, 'f', 3) + " Hz", m_pGroupBox_FlimControl);
    
    pHBoxLayout_FlimLaserTrigControl->addWidget(m_pCheckBox_FlimLaserTrigControl);
    pHBoxLayout_FlimLaserTrigControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_FlimLaserTrigControl->addWidget(m_pLabel_RepetitionRate);

    m_pVBoxLayout_FlimControl->addItem(pHBoxLayout_FlimLaserTrigControl);

    // Connect signal and slot
    connect(m_pCheckBox_FlimLaserTrigControl, SIGNAL(toggled(bool)), this, SLOT(startFlimTriggering(bool)));
}

void QDeviceControlTab::createFlimLaserControl()
{
    // Create widgets for FLIM laser power control
    QGridLayout *pGridLayout_FlimLaserPowerControl = new QGridLayout;
    pGridLayout_FlimLaserPowerControl->setSpacing(3);

    m_pCheckBox_FlimLaserControl = new QCheckBox(m_pGroupBox_FlimControl);
	m_pCheckBox_FlimLaserControl->setText("Connect to FLIm Laser");

	m_pLabel_FlimLaserPower = new QLabel(m_pGroupBox_FlimControl);
	m_pLabel_FlimLaserPower->setText("Power ");
	m_pLabel_FlimLaserPower->setEnabled(false);

	m_pSpinBox_FlimLaserPower = new QSpinBox(m_pGroupBox_FlimControl);
	m_pSpinBox_FlimLaserPower->setValue(m_pConfig->flimLaserPower);
	m_pSpinBox_FlimLaserPower->setRange(0, 255);
	m_pSpinBox_FlimLaserPower->setFixedWidth(50);
	m_pSpinBox_FlimLaserPower->setAlignment(Qt::AlignCenter);
	m_pSpinBox_FlimLaserPower->setDisabled(true);

    pGridLayout_FlimLaserPowerControl->addWidget(m_pCheckBox_FlimLaserControl, 0, 0);
    pGridLayout_FlimLaserPowerControl->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
	pGridLayout_FlimLaserPowerControl->addWidget(m_pLabel_FlimLaserPower, 0, 2);
    pGridLayout_FlimLaserPowerControl->addWidget(m_pSpinBox_FlimLaserPower, 0, 3);
	
	m_pVBoxLayout_FlimControl->addItem(pGridLayout_FlimLaserPowerControl);

	// Connect signal and slot
    connect(m_pCheckBox_FlimLaserControl, SIGNAL(toggled(bool)), this, SLOT(connectFlimLaser(bool)));
	connect(m_pSpinBox_FlimLaserPower, SIGNAL(valueChanged(int)), this, SLOT(adjustLaserPower(int)));
}

void QDeviceControlTab::createFlimCalibControl()
{
    // Create widgets for FLIm Calibration
    QGridLayout *pGridLayout_FlimCalibration = new QGridLayout;
    pGridLayout_FlimCalibration->setSpacing(3);

    m_pCheckBox_AlazarDigitizerControl = new QCheckBox(m_pGroupBox_FlimControl);
	m_pCheckBox_AlazarDigitizerControl->setText("Connect to Alazar Digitizer");
	m_pCheckBox_AlazarDigitizerControl->setDisabled(true);

    m_pPushButton_FlimCalibration = new QPushButton(this);
    m_pPushButton_FlimCalibration->setText("FLIm Pulse View and Calibration...");
    m_pPushButton_FlimCalibration->setFixedWidth(200);
    m_pPushButton_FlimCalibration->setDisabled(true);

    pGridLayout_FlimCalibration->addWidget(m_pCheckBox_AlazarDigitizerControl, 0, 0, 1, 3);
    pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_FlimCalibration->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_FlimCalibration->addWidget(m_pPushButton_FlimCalibration, 2, 1 , 1, 2);

    m_pVBoxLayout_FlimControl->addItem(pGridLayout_FlimCalibration);

    m_pGroupBox_FlimControl->setLayout(m_pVBoxLayout_FlimControl);
    m_pGroupBox_FlimControl->resize(m_pVBoxLayout_FlimControl->minimumSize());
    m_pVBoxLayout->addWidget(m_pGroupBox_FlimControl);

    // Connect signal and slot
    connect(m_pCheckBox_AlazarDigitizerControl, SIGNAL(toggled(bool)), this, SLOT(connectAlazarDigitizer(bool)));
    connect(m_pPushButton_FlimCalibration, SIGNAL(clicked(bool)), this, SLOT(createFlimCalibDlg()));
}

void QDeviceControlTab::createScanControl()
{
    // Create widgets for scanner control
    QGroupBox *pGroupBox_ScannerControl = new QGroupBox;
	QVBoxLayout *pVBoxLayout_ScannerControl = new QVBoxLayout;
	pVBoxLayout_ScannerControl->setSpacing(3);
	
	m_pCheckBox_ResonantScanControl = new QCheckBox(pGroupBox_ScannerControl);
	m_pCheckBox_ResonantScanControl->setText("Start Resonant Scanning");

	m_pToggledButton_ResonantScan = new QPushButton(pGroupBox_ScannerControl);
	m_pToggledButton_ResonantScan->setCheckable(true);
	m_pToggledButton_ResonantScan->setText("Voltage On");
	m_pToggledButton_ResonantScan->setDisabled(true);

	m_pDoubleSpinBox_ResonantScan = new QDoubleSpinBox(pGroupBox_ScannerControl);
	m_pDoubleSpinBox_ResonantScan->setFixedWidth(50);
	m_pDoubleSpinBox_ResonantScan->setRange(0.0, 5.0);
	m_pDoubleSpinBox_ResonantScan->setSingleStep(0.01);
	m_pDoubleSpinBox_ResonantScan->setValue(0.00);
	m_pDoubleSpinBox_ResonantScan->setDecimals(2);
	m_pDoubleSpinBox_ResonantScan->setAlignment(Qt::AlignCenter);
	m_pDoubleSpinBox_ResonantScan->setDisabled(true);

	m_pLabel_ResonantScan = new QLabel("V", pGroupBox_ScannerControl);
	m_pLabel_ResonantScan->setDisabled(true);
	
    m_pCheckBox_GalvoScanControl = new QCheckBox(pGroupBox_ScannerControl);
    m_pCheckBox_GalvoScanControl->setText("Start Galvano Mirror Scanning");

    m_pLineEdit_PeakToPeakVoltage = new QLineEdit(pGroupBox_ScannerControl);
    m_pLineEdit_PeakToPeakVoltage->setFixedWidth(30);
    m_pLineEdit_PeakToPeakVoltage->setText(QString::number(m_pConfig->galvoScanVoltage, 'f', 1));
    m_pLineEdit_PeakToPeakVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_PeakToPeakVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLineEdit_OffsetVoltage = new QLineEdit(pGroupBox_ScannerControl);
    m_pLineEdit_OffsetVoltage->setFixedWidth(30);
    m_pLineEdit_OffsetVoltage->setText(QString::number(m_pConfig->galvoScanVoltageOffset, 'f', 1));
    m_pLineEdit_OffsetVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_OffsetVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLabel_ScanVoltage = new QLabel("Scan Voltage ", pGroupBox_ScannerControl);
    m_pLabel_ScanPlusMinus = new QLabel("+", pGroupBox_ScannerControl);
    m_pLabel_ScanPlusMinus->setBuddy(m_pLineEdit_PeakToPeakVoltage);
    m_pLabel_GalvanoVoltage = new QLabel("V", pGroupBox_ScannerControl);
    m_pLabel_GalvanoVoltage->setBuddy(m_pLineEdit_OffsetVoltage);
			
	QGridLayout *pGridLayout_ResonantScanControl = new QGridLayout;
	pGridLayout_ResonantScanControl->setSpacing(3);

	pGridLayout_ResonantScanControl->addWidget(m_pCheckBox_ResonantScanControl, 0, 0, 1, 4);
	pGridLayout_ResonantScanControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_ResonantScanControl->addWidget(m_pToggledButton_ResonantScan, 1, 1);
	pGridLayout_ResonantScanControl->addWidget(m_pDoubleSpinBox_ResonantScan, 1, 2);
	pGridLayout_ResonantScanControl->addWidget(m_pLabel_ResonantScan, 1, 3);

	QGridLayout *pGridLayout_GalvoScanControl = new QGridLayout;
	pGridLayout_GalvoScanControl->setSpacing(3);

	pGridLayout_GalvoScanControl->addWidget(m_pCheckBox_GalvoScanControl, 0, 0, 1, 6);
	pGridLayout_GalvoScanControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
	pGridLayout_GalvoScanControl->addWidget(m_pLabel_ScanVoltage, 2, 1);
	pGridLayout_GalvoScanControl->addWidget(m_pLineEdit_PeakToPeakVoltage, 2, 2);
	pGridLayout_GalvoScanControl->addWidget(m_pLabel_ScanPlusMinus, 2, 3);
	pGridLayout_GalvoScanControl->addWidget(m_pLineEdit_OffsetVoltage, 2, 4);
	pGridLayout_GalvoScanControl->addWidget(m_pLabel_GalvanoVoltage, 2, 5);

	pVBoxLayout_ScannerControl->addItem(pGridLayout_ResonantScanControl);
	pVBoxLayout_ScannerControl->addItem(pGridLayout_GalvoScanControl);

	pGroupBox_ScannerControl->setLayout(pVBoxLayout_ScannerControl);
    m_pVBoxLayout->addWidget(pGroupBox_ScannerControl);

    // Connect signal and slot
	connect(m_pCheckBox_ResonantScanControl, SIGNAL(toggled(bool)), this, SLOT(connectResonantScanner(bool)));
	connect(m_pToggledButton_ResonantScan, SIGNAL(toggled(bool)), this, SLOT(applyResonantScannerVoltage(bool)));
	connect(m_pDoubleSpinBox_ResonantScan, SIGNAL(valueChanged(double)), this, SLOT(setResonantScannerVoltage(double)));
    connect(m_pCheckBox_GalvoScanControl, SIGNAL(toggled(bool)), this, SLOT(connectGalvanoMirror(bool)));
    connect(m_pLineEdit_PeakToPeakVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoScanVoltage(const QString &)));
    connect(m_pLineEdit_OffsetVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoScanVoltageOffset(const QString &)));    
}

void QDeviceControlTab::createZaberStageControl()
{
	// Create widgets for Zaber stage control
    QGroupBox *pGroupBox_ZaberStageControl = new QGroupBox;
    QGridLayout *pGridLayout_ZaberStageControl = new QGridLayout;
    pGridLayout_ZaberStageControl->setSpacing(3);

    m_pCheckBox_ZaberStageControl = new QCheckBox(pGroupBox_ZaberStageControl);
    m_pCheckBox_ZaberStageControl->setText("Connect to Zaber Stage");

	m_pPushButton_SetStageNumber = new QPushButton(pGroupBox_ZaberStageControl);
	m_pPushButton_SetStageNumber->setText("Stage Number");
	m_pPushButton_SetStageNumber->setDisabled(true);
    m_pPushButton_SetTargetSpeed = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_SetTargetSpeed->setText("Target Speed");
	m_pPushButton_SetTargetSpeed->setDisabled(true);
    m_pPushButton_MoveRelative = new QPushButton(pGroupBox_ZaberStageControl);
	m_pPushButton_MoveRelative->setText("Move Relative");
	m_pPushButton_MoveRelative->setDisabled(true);
    m_pPushButton_Home = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Home->setText("Home");
	m_pPushButton_Home->setFixedWidth(60);
	m_pPushButton_Home->setDisabled(true);
    m_pPushButton_Stop = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Stop->setText("Stop");
	m_pPushButton_Stop->setFixedWidth(60);
	m_pPushButton_Stop->setDisabled(true);

	m_pComboBox_StageNumber = new QComboBox(pGroupBox_ZaberStageControl);
	m_pComboBox_StageNumber->addItem("X-axis");
	m_pComboBox_StageNumber->addItem("Y-axis");
	m_pComboBox_StageNumber->setCurrentIndex(0);
	m_pComboBox_StageNumber->setDisabled(true);
    m_pLineEdit_TargetSpeed = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TargetSpeed->setFixedWidth(25);
    m_pLineEdit_TargetSpeed->setText(QString::number(m_pConfig->zaberPullbackSpeed));
	m_pLineEdit_TargetSpeed->setAlignment(Qt::AlignCenter);
	m_pLineEdit_TargetSpeed->setDisabled(true);
    m_pLineEdit_TravelLength = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TravelLength->setFixedWidth(25);
    m_pLineEdit_TravelLength->setText(QString::number(m_pConfig->zaberPullbackLength));
	m_pLineEdit_TravelLength->setAlignment(Qt::AlignCenter);
	m_pLineEdit_TravelLength->setDisabled(true);

    m_pLabel_TargetSpeed = new QLabel("mm/s", pGroupBox_ZaberStageControl);
    m_pLabel_TargetSpeed->setBuddy(m_pLineEdit_TargetSpeed);
	m_pLabel_TargetSpeed->setDisabled(true);
    m_pLabel_TravelLength = new QLabel("mm", pGroupBox_ZaberStageControl);
    m_pLabel_TravelLength->setBuddy(m_pLineEdit_TravelLength);
	m_pLabel_TravelLength->setDisabled(true);
	
    pGridLayout_ZaberStageControl->addWidget(m_pCheckBox_ZaberStageControl, 0, 0, 1, 5);
	
	pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_ZaberStageControl->addWidget(m_pPushButton_SetStageNumber, 1, 1, 1, 2);
	pGridLayout_ZaberStageControl->addWidget(m_pComboBox_StageNumber, 1, 3, 1, 2);

    //pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_SetTargetSpeed, 2, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TargetSpeed, 2, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TargetSpeed, 2, 4);

    //pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_MoveRelative, 3, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TravelLength, 3, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TravelLength, 3, 4);

    //pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 4, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Home, 4, 1);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Stop, 4, 2);	

	pGroupBox_ZaberStageControl->setLayout(pGridLayout_ZaberStageControl);
	m_pVBoxLayout->addWidget(pGroupBox_ZaberStageControl);

	// Connect signal and slot
	connect(m_pCheckBox_ZaberStageControl, SIGNAL(toggled(bool)), this, SLOT(connectZaberStage(bool)));
	connect(m_pPushButton_MoveRelative, SIGNAL(clicked(bool)), this, SLOT(moveRelative()));
	connect(m_pLineEdit_TargetSpeed, SIGNAL(textChanged(const QString &)), this, SLOT(setTargetSpeed(const QString &)));
	connect(m_pLineEdit_TravelLength, SIGNAL(textChanged(const QString &)), this, SLOT(changeZaberPullbackLength(const QString &)));
	connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
	connect(m_pPushButton_Stop, SIGNAL(clicked(bool)), this, SLOT(stop()));
}


void QDeviceControlTab::applyPmtGainVoltage(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_PmtGainControl->setText("Stop Applying PMT Gain Control");

        // Set enabled false for PMT gain control widgets
        m_pLineEdit_PmtGainVoltage->setEnabled(false);
        m_pLabel_PmtGainVoltage->setEnabled(false);

        // Create PMT gain control objects
        if (!m_pPmtGainControl)
        {
            m_pPmtGainControl = new PmtGainControl;
            m_pPmtGainControl->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

        m_pPmtGainControl->voltage = m_pLineEdit_PmtGainVoltage->text().toDouble();
        if (m_pPmtGainControl->voltage > 1.0)
        {
            m_pPmtGainControl->SendStatusMessage(">1.0V Gain cannot be assigned!", true);
            m_pCheckBox_PmtGainControl->setChecked(false);
            return;
        }

        // Initializing
        if (!m_pPmtGainControl->initialize())
        {
            m_pCheckBox_PmtGainControl->setChecked(false);
            return;
        }

        // Generate PMT gain voltage
        m_pPmtGainControl->start();
    }
    else
    {
        // Delete PMT gain control objects
        if (m_pPmtGainControl)
        {
            m_pPmtGainControl->stop();
            delete m_pPmtGainControl;
            m_pPmtGainControl = nullptr;
        }

        // Set enabled true for PMT gain control widgets
        m_pLineEdit_PmtGainVoltage->setEnabled(true);
        m_pLabel_PmtGainVoltage->setEnabled(true);

        // Set text
        m_pCheckBox_PmtGainControl->setText("Apply PMT Gain Control");
    }
}

void QDeviceControlTab::changePmtGainVoltage(const QString &vol)
{
    m_pConfig->pmtGainVoltage = vol.toFloat();
}

void QDeviceControlTab::startFlimTriggering(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_FlimLaserTrigControl->setText("Stop FLIm Laser Triggering");

        // Set enabled false for FLIm laser sync control widgets
        m_pLabel_RepetitionRate->setEnabled(false);

        // Create FLIm laser sync control objects
        if (!m_pFlimTrigControl)
        {
            m_pFlimTrigControl = new FlimTrigger;
            m_pFlimTrigControl->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

        m_pFlimTrigControl->frequency = FLIM_LASER_REP_RATE; 
		m_pFlimTrigControl->finite_samps = FLIM_LASER_FINITE_SAMPS;
		
        // Initializing
        if (!m_pFlimTrigControl->initialize())
        {
            m_pCheckBox_FlimLaserTrigControl->setChecked(false);
            return;
        }

        // Generate FLIm laser sync
        m_pFlimTrigControl->start();
    }
    else
    {
        // Delete FLIm laser sync control objects
        if (m_pFlimTrigControl)
        {
            m_pFlimTrigControl->stop();
            delete m_pFlimTrigControl;
            m_pFlimTrigControl = nullptr;
        }

        // Set enabled true for FLIm laser sync control widgets
        m_pLabel_RepetitionRate->setEnabled(true);


        // Set text
        m_pCheckBox_FlimLaserTrigControl->setText("Start FLIm Laser Triggering");
    }
}

void QDeviceControlTab::changeFlimLaserRepRate(const QString &rep_rate)
{
    m_pConfig->flimLaserRepRate = rep_rate.toInt();
}

void QDeviceControlTab::connectFlimLaser(bool toggled)
{
	if (toggled)
	{
		// Set text
        m_pCheckBox_FlimLaserControl->setText("Disconnect from FLIm Laser");

		// Create FLIM laser power control objects
		if (!m_pIPGPhotonicsLaser)
		{
			m_pIPGPhotonicsLaser = new IPGPhotonicsLaser;
			m_pIPGPhotonicsLaser->SetPortName(FLIM_LASER_COM_PORT);

			m_pIPGPhotonicsLaser->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg); 
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Connect the laser
		if (!(m_pIPGPhotonicsLaser->ConnectDevice()))
		{
			m_pCheckBox_FlimLaserControl->setChecked(false);
			return;
		}
		else
		{
			// Default setup
			m_pIPGPhotonicsLaser->SetIntTrigMode(false);
			m_pIPGPhotonicsLaser->SetIntModulation(false);
			m_pIPGPhotonicsLaser->SetIntPowerControl(false);
			m_pIPGPhotonicsLaser->SetIntEmissionControl(false);

			adjustLaserPower(m_pConfig->flimLaserPower);

			if (!m_pIPGPhotonicsLaser->EnableEmissionD(true))
			{
				m_pCheckBox_FlimLaserControl->setChecked(false);
				return;
			}

			m_pIPGPhotonicsLaser->ReadStatus();
		}

		// Set enabled true for FLIM laser power control widgets
		m_pLabel_FlimLaserPower->setEnabled(true);
		m_pSpinBox_FlimLaserPower->setEnabled(true);
	}
	else
	{
		// Set enabled false for FLIM laser power control widgets
		m_pLabel_FlimLaserPower->setEnabled(false);
		m_pSpinBox_FlimLaserPower->setEnabled(false);

		if (m_pIPGPhotonicsLaser)
		{
			// Disconnect the laser
			m_pIPGPhotonicsLaser->DisconnectDevice();

			// Delete FLIM laser power control objects
			delete m_pIPGPhotonicsLaser;
			m_pIPGPhotonicsLaser = nullptr;
		}
		
		// Set text
        m_pCheckBox_FlimLaserControl->setText("Connect to FLIm Laser");
	}
}

void QDeviceControlTab::adjustLaserPower(int level)
{
	m_pConfig->flimLaserPower = level;

	if (m_pIPGPhotonicsLaser)
		m_pIPGPhotonicsLaser->SetPowerLevel((uint8_t)level);
}


void QDeviceControlTab::connectAlazarDigitizer(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_AlazarDigitizerControl->setText("Disconnect from Alazar Digitizer");
				
        // Set enabled true for Alazar digitizer widgets
        m_pPushButton_FlimCalibration->setEnabled(true);
    }
    else
    {
        // Set disabled true for Alazar digitizer widgets
        m_pPushButton_FlimCalibration->setDisabled(true);

        // Close FLIm calibration window
        if (m_pFlimCalibDlg) m_pFlimCalibDlg->close();

        // Set text
        m_pCheckBox_AlazarDigitizerControl->setText("Connect to Alazar Digitizer");
    }
}

void QDeviceControlTab::createFlimCalibDlg()
{
    if (m_pFlimCalibDlg == nullptr)
    {
        m_pFlimCalibDlg = new FlimCalibDlg(this);
        connect(m_pFlimCalibDlg, SIGNAL(finished(int)), this, SLOT(deleteFlimCalibDlg()));
        m_pFlimCalibDlg->show();
//        emit m_pFlimCalibDlg->plotRoiPulse(m_pFLIm, m_pSlider_SelectAline->value() / 4);
    }
    m_pFlimCalibDlg->raise();
    m_pFlimCalibDlg->activateWindow();

	m_pStreamTab->getVisualizationTab()->setImgViewVisPixelPos(true);
}

void QDeviceControlTab::deleteFlimCalibDlg()
{
	m_pStreamTab->getVisualizationTab()->setImgViewVisPixelPos(false);

//    m_pFlimCalibDlg->showWindow(false);
//    m_pFlimCalibDlg->showMeanDelay(false);
//    m_pFlimCalibDlg->showMask(false);

    m_pFlimCalibDlg->deleteLater();
    m_pFlimCalibDlg = nullptr;
}


void QDeviceControlTab::connectResonantScanner(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_ResonantScanControl->setText("Stop Resonant Scanning");

		// Set enable true for voltage control widgets
		m_pToggledButton_ResonantScan->setEnabled(true);
	}
	else
	{
		// Voltage control off
		m_pToggledButton_ResonantScan->setChecked(false);

		// Set enable false for voltage control widgets
		m_pToggledButton_ResonantScan->setEnabled(false);
		m_pDoubleSpinBox_ResonantScan->setEnabled(false);
		m_pLabel_ResonantScan->setEnabled(false);

		// Set text
		m_pCheckBox_ResonantScanControl->setText("Start Resonant Scanning");
	}
}

void QDeviceControlTab::applyResonantScannerVoltage(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pToggledButton_ResonantScan->setText("Voltage Off");

		// Create resonant scanner voltage control objects
		if (!m_pResonantScan)
		{
			m_pResonantScan = new ResonantScan;
			m_pResonantScan->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Initializing
		if (!m_pResonantScan->initialize())
		{
			m_pToggledButton_ResonantScan->setChecked(false);
			return;
		}

		// Apply zero voltage
		m_pResonantScan->apply(0);
		//m_pConfig->resonantScanVoltage = 0;

		// Set enable true for resonant scanner voltage control
		m_pDoubleSpinBox_ResonantScan->setEnabled(true);
		m_pLabel_ResonantScan->setEnabled(true);
	}
	else
	{
		// Set enable false for resonant scanner voltage control
		m_pDoubleSpinBox_ResonantScan->setValue(0.0);
		m_pDoubleSpinBox_ResonantScan->setEnabled(false);
		m_pLabel_ResonantScan->setEnabled(false);

		// Delete resonant scanner voltage control objects
		if (m_pResonantScan)
		{
			m_pResonantScan->apply(0);
			//m_pConfig->resonantScanVoltage = 0;

			delete m_pResonantScan;
			m_pResonantScan = nullptr;
		}

		// Set text
		m_pToggledButton_ResonantScan->setText("Voltage On");
	}
}

void QDeviceControlTab::setResonantScannerVoltage(double voltage)
{
	// Generate voltage for resonant scanner control
	if (m_pResonantScan)
		m_pResonantScan->apply(voltage);
	if (voltage != 0.00)
		m_pConfig->resonantScanVoltage = voltage;
}

void QDeviceControlTab::connectGalvanoMirror(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_GalvoScanControl->setText("Stop Galvano Mirror Scanning");

        // Set enabled false for Galvano mirror control widgets
		m_pLabel_ScanVoltage->setEnabled(false);
		m_pLineEdit_PeakToPeakVoltage->setEnabled(false);
		m_pLabel_ScanPlusMinus->setEnabled(false);
		m_pLineEdit_OffsetVoltage->setEnabled(false);
		m_pLabel_GalvanoVoltage->setEnabled(false);

        m_pStreamTab->setYLinesWidgets(false);

        // Create Galvano mirror control objects
        if (!m_pGalvoScan)
        {
            m_pGalvoScan = new GalvoScan;
            m_pGalvoScan->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

		m_pGalvoScan->triggerSource = NI_GALVO_START_TRIG_SOURCE;
        m_pGalvoScan->pp_voltage = m_pLineEdit_PeakToPeakVoltage->text().toDouble();
        m_pGalvoScan->offset = m_pLineEdit_OffsetVoltage->text().toDouble();        
		m_pGalvoScan->step = m_pConfig->nLines + GALVO_FLYING_BACK;

        // Initializing
		if (!m_pGalvoScan->initialize())
		{
			m_pCheckBox_GalvoScanControl->setChecked(false);
			return;
		}
		else
			m_pGalvoScan->start();		
    }
    else
    {
        // Delete Galvano mirror control objects
        if (m_pGalvoScan)
        {
            m_pGalvoScan->stop();
            delete m_pGalvoScan;
            m_pGalvoScan = nullptr;
        }

        // Set enabled false for Galvano mirror control widgets
		m_pLabel_ScanVoltage->setEnabled(true);
		m_pLineEdit_PeakToPeakVoltage->setEnabled(true);
		m_pLabel_ScanPlusMinus->setEnabled(true);
		m_pLineEdit_OffsetVoltage->setEnabled(true);
		m_pLabel_GalvanoVoltage->setEnabled(true);

		if (!m_pStreamTab->getOperationTab()->isAcquisitionButtonToggled())
			m_pStreamTab->setYLinesWidgets(true);

        // Set text
        m_pCheckBox_GalvoScanControl->setText("Start Galvano Mirror Scanning");
    }
}

void QDeviceControlTab::changeGalvoScanVoltage(const QString &vol)
{
	m_pConfig->galvoScanVoltage = vol.toFloat();
}

void QDeviceControlTab::changeGalvoScanVoltageOffset(const QString &vol)
{
	m_pConfig->galvoScanVoltageOffset = vol.toFloat();
}


void QDeviceControlTab::connectZaberStage(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_ZaberStageControl->setText("Disconnect from Zaber Stage");

		// Create Zaber stage control objects
		if (!m_pZaberStage)
		{
			m_pZaberStage = new ZaberStage;
			m_pZaberStage->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');	
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Connect stage
		if (!(m_pZaberStage->ConnectStage()))
		{
			m_pCheckBox_ZaberStageControl->setChecked(false);
			return;
		}

		// Set callback function
		m_pZaberStage->DidMovedRelative += [&]() {
			if (m_pStreamTab->getOperationTab()->m_pMemoryBuffer->m_bIsRecording)
			{
				if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
				{					
					m_pCheckBox_FlimLaserTrigControl->setChecked(true);

					std::unique_lock<std::mutex> mlock(m_mtxStageScan);
					mlock.unlock();
					m_cvStageScan.notify_one();
				}
			}
		};

		// Set target speed first
		m_pZaberStage->SetTargetSpeed(1, m_pLineEdit_TargetSpeed->text().toDouble());
		m_pZaberStage->SetTargetSpeed(2, m_pLineEdit_TargetSpeed->text().toDouble());

		// Set enable true for Zaber stage control widgets
		m_pPushButton_SetStageNumber->setEnabled(true);
		m_pPushButton_MoveRelative->setEnabled(true);
		m_pPushButton_SetTargetSpeed->setEnabled(true);
		m_pPushButton_Home->setEnabled(true);
		m_pPushButton_Stop->setEnabled(true);
		m_pComboBox_StageNumber->setEnabled(true);
		m_pLineEdit_TravelLength->setEnabled(true);
		m_pLineEdit_TargetSpeed->setEnabled(true);
		m_pLabel_TravelLength->setEnabled(true);
		m_pLabel_TargetSpeed->setEnabled(true);

		m_pStreamTab->getImageStitchingCheckBox()->setEnabled(true);
	}
	else
	{
		// Set enable false for Zaber stage control widgets
		if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
			m_pStreamTab->getImageStitchingCheckBox()->setChecked(false);
		m_pStreamTab->getImageStitchingCheckBox()->setDisabled(true);

		m_pPushButton_SetStageNumber->setEnabled(false);
		m_pPushButton_MoveRelative->setEnabled(false);
		m_pPushButton_SetTargetSpeed->setEnabled(false);
		m_pPushButton_Home->setEnabled(false);
		m_pPushButton_Stop->setEnabled(false);
		m_pComboBox_StageNumber->setEnabled(false);
		m_pLineEdit_TravelLength->setEnabled(false);
		m_pLineEdit_TargetSpeed->setEnabled(false);
		m_pLabel_TravelLength->setEnabled(false);
		m_pLabel_TargetSpeed->setEnabled(false);

		if (m_pZaberStage)
		{
			// Stop Wait Thread
			m_pZaberStage->StopWaitThread();

			// Disconnect the Stage
			m_pZaberStage->DisconnectStage();

			// Delete Zaber stage control objects
			delete m_pZaberStage;
			m_pZaberStage = nullptr;
		}

		// Set text
		m_pCheckBox_ZaberStageControl->setText("Connect to Zaber Stage");
	}
}

void QDeviceControlTab::moveRelative()
{
	m_pZaberStage->MoveRelative(m_pComboBox_StageNumber->currentIndex() + 1, m_pLineEdit_TravelLength->text().toDouble());
}

void QDeviceControlTab::setTargetSpeed(const QString & str)
{
	m_pZaberStage->SetTargetSpeed(m_pComboBox_StageNumber->currentIndex() + 1, str.toDouble());
	m_pConfig->zaberPullbackSpeed = str.toInt();
}

void QDeviceControlTab::changeZaberPullbackLength(const QString &str)
{
	m_pConfig->zaberPullbackLength = str.toFloat();
}

void QDeviceControlTab::home()
{
	m_pZaberStage->Home(m_pComboBox_StageNumber->currentIndex() + 1);
}

void QDeviceControlTab::stop()
{
	m_pZaberStage->Stop();
}