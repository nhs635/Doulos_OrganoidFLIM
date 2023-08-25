
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

#include <DeviceControl/ResonantScan/PulseTrainGenerator.h>
#include <DeviceControl/ResonantScan/ResonantScan.h>
#include <DeviceControl/GalvoScan/GalvoScan.h>
#if (CRS_DIR_FACTOR == 2)
#include <DeviceControl/GalvoScan/TwoEdgeTriggerEnable.h>
#endif
#include <DeviceControl/NanoscopeStage/NanoscopeStage.h>
#include <DeviceControl/DpcIllumination/DpcIllumination.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <Common/Array.h>

#include <iostream>
#include <thread>


QDeviceControlTab::QDeviceControlTab(QWidget *parent) :
    QDialog(parent), m_pPmtGainControl(nullptr), m_pFlimTrigControlLaser(nullptr), m_pFlimTrigControlDAQ(nullptr),
    m_pIPGPhotonicsLaser(nullptr), m_pFlimCalibDlg(nullptr), 
	m_pGalvoScan(nullptr), m_pNanoscopeStage(nullptr), m_pDpcIllumControl(nullptr)
#if (CRS_DIR_FACTOR == 2)
	, m_pTwoEdgeTriggerEnable(nullptr)
#endif
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
	createNanoscopeStageControl();
	createDpcIlluminationControl();
	
    // Set layout
    setLayout(m_pVBoxLayout);
}

QDeviceControlTab::~QDeviceControlTab()
{
    if (m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(false);
    if (m_pCheckBox_FlimLaserTrigControl->isChecked()) m_pCheckBox_FlimLaserTrigControl->setChecked(false);
    if (m_pCheckBox_FlimLaserControl->isChecked()) m_pCheckBox_FlimLaserControl->setChecked(false);	
	if (m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(false);
	if (m_pCheckBox_NanoscopeStageControl->isChecked()) m_pCheckBox_NanoscopeStageControl->setChecked(false);
	if (m_pDpcIllumControl) connectDpcIllumination(false);
	
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
		if (m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(false);
		if (m_pCheckBox_NanoscopeStageControl->isChecked()) m_pCheckBox_NanoscopeStageControl->setChecked(false);
		if (m_pDpcIllumControl) connectDpcIllumination(false);
	}
	else
	{
		if (!m_pCheckBox_PmtGainControl->isChecked()) m_pCheckBox_PmtGainControl->setChecked(true);
		if (!m_pCheckBox_FlimLaserTrigControl->isChecked()) m_pCheckBox_FlimLaserTrigControl->setChecked(true);
		if (!m_pCheckBox_FlimLaserControl->isChecked()) m_pCheckBox_FlimLaserControl->setChecked(true);		
		if (!m_pCheckBox_GalvoScanControl->isChecked()) m_pCheckBox_GalvoScanControl->setChecked(true);
		if (!m_pCheckBox_NanoscopeStageControl->isChecked()) m_pCheckBox_NanoscopeStageControl->setChecked(true);
		if (!m_pDpcIllumControl) connectDpcIllumination(true);
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
    //m_pPushButton_FlimCalibration->setDisabled(true);

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
    m_pGroupBox_ScannerControl = new QGroupBox;
	QVBoxLayout *pVBoxLayout_ScannerControl = new QVBoxLayout;
	pVBoxLayout_ScannerControl->setSpacing(3);
		
    m_pCheckBox_GalvoScanControl = new QCheckBox(m_pGroupBox_ScannerControl);
    m_pCheckBox_GalvoScanControl->setText("Start Galvano Mirror Scanning");
	
	m_pLineEdit_GalvoFastScanFreq = new QLineEdit(m_pGroupBox_ScannerControl);
	m_pLineEdit_GalvoFastScanFreq->setFixedWidth(35);
	m_pLineEdit_GalvoFastScanFreq->setText(QString::number(FAST_SCAN_FREQ));	
	m_pLineEdit_GalvoFastScanFreq->setAlignment(Qt::AlignCenter);
	
    m_pLineEdit_PeakToPeakVoltage = new QLineEdit(m_pGroupBox_ScannerControl);
    m_pLineEdit_PeakToPeakVoltage->setFixedWidth(30);
    m_pLineEdit_PeakToPeakVoltage->setText(QString::number(m_pConfig->galvoScanVoltage, 'f', 1));
    m_pLineEdit_PeakToPeakVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_PeakToPeakVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLineEdit_OffsetVoltage = new QLineEdit(m_pGroupBox_ScannerControl);
    m_pLineEdit_OffsetVoltage->setFixedWidth(30);
    m_pLineEdit_OffsetVoltage->setText(QString::number(m_pConfig->galvoScanVoltageOffset, 'f', 1));
    m_pLineEdit_OffsetVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_OffsetVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		
	m_pLabel_GalvoFastScanFreq = new QLabel("Scan Freq ", m_pGroupBox_ScannerControl);
	m_pLabel_GalvoFastScanFreqHz = new QLabel("Hz", m_pGroupBox_ScannerControl);
    m_pLabel_ScanVoltage = new QLabel("Scan Voltage ", m_pGroupBox_ScannerControl);
    m_pLabel_ScanPlusMinus = new QLabel("+", m_pGroupBox_ScannerControl);
    m_pLabel_ScanPlusMinus->setBuddy(m_pLineEdit_PeakToPeakVoltage);
    m_pLabel_GalvanoVoltage = new QLabel("V", m_pGroupBox_ScannerControl);
    m_pLabel_GalvanoVoltage->setBuddy(m_pLineEdit_OffsetVoltage);

	QHBoxLayout *pHBoxLayout_FastScanFreq = new QHBoxLayout;
	pHBoxLayout_FastScanFreq->setSpacing(3);

	pHBoxLayout_FastScanFreq->addWidget(m_pCheckBox_GalvoScanControl);
	pHBoxLayout_FastScanFreq->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_FastScanFreq->addWidget(m_pLabel_GalvoFastScanFreq);
	pHBoxLayout_FastScanFreq->addWidget(m_pLineEdit_GalvoFastScanFreq);
	pHBoxLayout_FastScanFreq->addWidget(m_pLabel_GalvoFastScanFreqHz);
	
	QGridLayout *pGridLayout_GalvoScanControl = new QGridLayout;
	pGridLayout_GalvoScanControl->setSpacing(3);

	pGridLayout_GalvoScanControl->addItem(pHBoxLayout_FastScanFreq, 0, 0, 1, 6);
	pGridLayout_GalvoScanControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_GalvoScanControl->addWidget(m_pLabel_ScanVoltage, 1, 1);
	pGridLayout_GalvoScanControl->addWidget(m_pLineEdit_PeakToPeakVoltage, 1, 2);
	pGridLayout_GalvoScanControl->addWidget(m_pLabel_ScanPlusMinus, 1, 3);
	pGridLayout_GalvoScanControl->addWidget(m_pLineEdit_OffsetVoltage, 1, 4);
	pGridLayout_GalvoScanControl->addWidget(m_pLabel_GalvanoVoltage, 1, 5);

	pVBoxLayout_ScannerControl->addItem(pGridLayout_GalvoScanControl);

	m_pGroupBox_ScannerControl->setLayout(pVBoxLayout_ScannerControl);
    m_pVBoxLayout->addWidget(m_pGroupBox_ScannerControl);

    // Connect signal and slot
    connect(m_pCheckBox_GalvoScanControl, SIGNAL(toggled(bool)), this, SLOT(connectGalvanoMirror(bool)));
    connect(m_pLineEdit_PeakToPeakVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoScanVoltage(const QString &)));
    connect(m_pLineEdit_OffsetVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoScanVoltageOffset(const QString &)));    
}

void QDeviceControlTab::createNanoscopeStageControl()
{
	// Create widgets for Nanoscope stage control
    m_pGroupBox_NanoscopeStageControl = new QGroupBox;
    QGridLayout *pGridLayout_NanoscopeStageControl = new QGridLayout;
    pGridLayout_NanoscopeStageControl->setSpacing(1);
	
	m_pCheckBox_NanoscopeStageControl = new QCheckBox(m_pGroupBox_NanoscopeStageControl);
	m_pCheckBox_NanoscopeStageControl->setText("Connect to Nanoscope Stage");

	// Data conversion
	m_pLabel_StageUnit = new QLabel(this);
	m_pLabel_StageUnit->setText("unit: mm");
	m_pLabel_StageUnit->setDisabled(true);

	// XYZ-axis widgets
	for (int i = 0; i < 3; i++)
	{
		m_pGroupBox_Axis[i] = new QGroupBox(m_pGroupBox_NanoscopeStageControl);
		m_pGroupBox_Axis[i]->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
		m_pGroupBox_Axis[i]->setFixedHeight(118);
		m_pGroupBox_Axis[i]->setTitle(QString("%1-axis").arg((char)('X'+i)));
		m_pGroupBox_Axis[i]->setDisabled(true);
		
		m_pToggleButton_JogCW[i] = new QPushButton(m_pGroupBox_NanoscopeStageControl);
		m_pToggleButton_JogCW[i]->setCheckable(true);
		m_pToggleButton_JogCW[i]->setText(i != 2 ? "<<" : "^^");
		m_pToggleButton_JogCW[i]->setStyleSheet("QPushButton{font-size: 7pt;}");
		m_pToggleButton_JogCW[i]->setFixedSize(19, 20);
		m_pPushButton_RelCW[i] = new QPushButton(m_pGroupBox_NanoscopeStageControl);
		m_pPushButton_RelCW[i]->setText(i != 2 ? "<" : "^");
		m_pPushButton_RelCW[i]->setStyleSheet("QPushButton{font-size: 7pt;}");
		m_pPushButton_RelCW[i]->setFixedSize(19, 20);			
		m_pPushButton_RelCCW[i] = new QPushButton(m_pGroupBox_NanoscopeStageControl);
		m_pPushButton_RelCCW[i]->setText(i != 2 ? ">" : "v");
		m_pPushButton_RelCCW[i]->setStyleSheet("QPushButton{font-size: 7pt;}");
		m_pPushButton_RelCCW[i]->setFixedSize(19, 20);
		m_pToggleButton_JogCCW[i] = new QPushButton(m_pGroupBox_NanoscopeStageControl);
		m_pToggleButton_JogCCW[i]->setCheckable(true);
		m_pToggleButton_JogCCW[i]->setText(i != 2 ? ">>" : "vv");
		m_pToggleButton_JogCCW[i]->setStyleSheet("QPushButton{font-size: 7pt;}");
		m_pToggleButton_JogCCW[i]->setFixedSize(19, 20);

		// Set step-size indicator
		m_pLabel_Step[i] = new QLabel(m_pGroupBox_NanoscopeStageControl);
		m_pLabel_Step[i]->setText("Step");
		m_pLabel_Step[i]->setStyleSheet("QLabel{font-size: 7pt;}");

		m_pLineEdit_Step[i] = new QLineEdit(m_pGroupBox_NanoscopeStageControl);		
		m_pLineEdit_Step[i]->setText(QString::number((double)(m_pConfig->NanoscopeStep[i] / (double)1'000'000.0), 'f', 4));		
		m_pLineEdit_Step[i]->setStyleSheet("QLineEdit{font-size: 7pt;}");
		m_pLineEdit_Step[i]->setAlignment(Qt::AlignCenter);
		m_pLineEdit_Step[i]->setFixedSize(42, 15);

		// Set position indicator
		m_pLabel_Pos[i] = new QLabel(m_pGroupBox_NanoscopeStageControl);
		m_pLabel_Pos[i]->setText("Pos");
		m_pLabel_Pos[i]->setStyleSheet("QLabel{font-size: 7pt;}");
		
		m_pLineEdit_Pos[i] = new QLineEdit(m_pGroupBox_NanoscopeStageControl);
		m_pLineEdit_Pos[i]->setText(QString::number((double)(m_pConfig->NanoscopePosition[i]) / 1'000'000.0, 'f', 4));		
		m_pLineEdit_Pos[i]->setStyleSheet("QLineEdit{font-size: 7pt;}");
		m_pLineEdit_Pos[i]->setAlignment(Qt::AlignCenter);
		m_pLineEdit_Pos[i]->setFixedSize(42, 15);

		// Set speed indicator
		m_pLabel_Speed[i] = new QLabel(m_pGroupBox_NanoscopeStageControl);
		m_pLabel_Speed[i]->setText("Prec ");
		m_pLabel_Speed[i]->setStyleSheet("QLabel{font-size: 7pt;}");

		m_pLineEdit_Speed[i] = new QLineEdit(m_pGroupBox_NanoscopeStageControl);
		m_pLineEdit_Speed[i]->setText(QString::number(m_pConfig->NanoscopeSpeed[i]));
		m_pLineEdit_Speed[i]->setStyleSheet("QLineEdit{font-size: 7pt;}");
		m_pLineEdit_Speed[i]->setAlignment(Qt::AlignCenter);
		m_pLineEdit_Speed[i]->setFixedSize(42, 15);

		// Set other buttons
		m_pPushButton_Read[i] = new QPushButton(m_pGroupBox_NanoscopeStageControl);
		m_pPushButton_Read[i]->setText("Read");
		m_pPushButton_Read[i]->setStyleSheet("QPushButton{font-size: 7pt;}");
		m_pPushButton_Read[i]->setFixedSize(35, 20);
		m_pPushButton_Stop[i] = new QPushButton(m_pGroupBox_NanoscopeStageControl);
		m_pPushButton_Stop[i]->setText("Stop");
		m_pPushButton_Stop[i]->setStyleSheet("QPushButton{font-size: 7pt;}");
		m_pPushButton_Stop[i]->setFixedSize(35, 20);
		
		QGridLayout *pGridLayout_Axis = new QGridLayout;
		pGridLayout_Axis->setSpacing(1);
		pGridLayout_Axis->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding), 0, 0, 1, 2);

		QHBoxLayout *pHBoxLayout_Navigation = new QHBoxLayout;
		pHBoxLayout_Navigation->setSpacing(0);
		pHBoxLayout_Navigation->addWidget(m_pToggleButton_JogCW[i]);
		pHBoxLayout_Navigation->addWidget(m_pPushButton_RelCW[i]);
		pHBoxLayout_Navigation->addWidget(m_pPushButton_RelCCW[i]);
		pHBoxLayout_Navigation->addWidget(m_pToggleButton_JogCCW[i]);

		pGridLayout_Axis->addItem(pHBoxLayout_Navigation, 1, 0, 1, 2);
		pGridLayout_Axis->addWidget(m_pLabel_Step[i], 2, 0);
		pGridLayout_Axis->addWidget(m_pLineEdit_Step[i], 2, 1);
		pGridLayout_Axis->addWidget(m_pLabel_Pos[i], 3, 0);
		pGridLayout_Axis->addWidget(m_pLineEdit_Pos[i], 3, 1);		
		pGridLayout_Axis->addWidget(m_pLabel_Speed[i], 4, 0);
		pGridLayout_Axis->addWidget(m_pLineEdit_Speed[i], 4, 1);

		QHBoxLayout *pHBoxLayout_Buttons = new QHBoxLayout;
		pHBoxLayout_Buttons->setSpacing(0);
		pHBoxLayout_Buttons->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
		pHBoxLayout_Buttons->addWidget(m_pPushButton_Read[i]);
		pHBoxLayout_Buttons->addWidget(m_pPushButton_Stop[i]);

		pGridLayout_Axis->addItem(pHBoxLayout_Buttons, 5, 0, 1, 2, Qt::AlignRight);
		m_pGroupBox_Axis[i]->setLayout(pGridLayout_Axis);

		pGridLayout_NanoscopeStageControl->addWidget(m_pGroupBox_Axis[i], 1, i);
	}
	
	// Set layout	
	QHBoxLayout *pHBoxLayout_DataType = new QHBoxLayout;
	pHBoxLayout_DataType->setSpacing(1);
	pHBoxLayout_DataType->addWidget(m_pCheckBox_NanoscopeStageControl);
	pHBoxLayout_DataType->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_DataType->addWidget(m_pLabel_StageUnit);
	
	pGridLayout_NanoscopeStageControl->addItem(pHBoxLayout_DataType, 0, 0, 1, 3);
	
	m_pGroupBox_NanoscopeStageControl->setLayout(pGridLayout_NanoscopeStageControl);
	m_pVBoxLayout->addWidget(m_pGroupBox_NanoscopeStageControl);

	// Connect signal and slot
	connect(m_pCheckBox_NanoscopeStageControl, SIGNAL(toggled(bool)), this, SLOT(connectNanoscopeStage(bool)));
		
	for (int i = 0; i < 3; i++)
	{
		connect(m_pToggleButton_JogCW[i], &QPushButton::toggled, [&, i](bool enabled) {
			if (enabled)
				m_pNanoscopeStage->writeStage(i + 1, rel_cw, -1);
			else
				m_pNanoscopeStage->writeStage(i + 1, stop, 0);
		});

		connect(m_pPushButton_RelCW[i], &QPushButton::clicked, [&, i]() {			
			m_pNanoscopeStage->writeStage(i + 1, rel_cw, m_pConfig->NanoscopeStep[i]); // data : nm
		});

		connect(m_pPushButton_RelCCW[i], &QPushButton::clicked, [&, i]() {
			m_pNanoscopeStage->writeStage(i + 1, rel_ccw, m_pConfig->NanoscopeStep[i]); // data : nm
		});

		connect(m_pToggleButton_JogCCW[i], &QPushButton::toggled, [&, i](bool enabled) {
			if (enabled)
				m_pNanoscopeStage->writeStage(i + 1, rel_ccw, -1);
			else
				m_pNanoscopeStage->writeStage(i + 1, stop, 0);
		});

		connect(m_pLineEdit_Pos[i], &QLineEdit::editingFinished, [&, i]() {
			// no operation (no absolute moving)
		});

		connect(m_pLineEdit_Step[i], &QLineEdit::textEdited, [&, i](const QString &str) {
			double step_size = str.toDouble();
			m_pConfig->NanoscopeStep[i] = (int)(step_size * 1'000'000);
		});

		connect(m_pLineEdit_Speed[i], &QLineEdit::textEdited, [&, i](const QString &str) {
			double _speed = str.toDouble();
			m_pConfig->NanoscopeSpeed[i] = (int)_speed; // *1'000'000);
			m_pNanoscopeStage->writeStage(i + 1, speed, m_pConfig->NanoscopeSpeed[i]);
		});

		connect(m_pPushButton_Read[i], &QPushButton::clicked, [&, i]() {
			m_pNanoscopeStage->writeStage(i + 1, pos_que, 0);
		});

		connect(m_pPushButton_Stop[i], &QPushButton::clicked, [&, i]() {
			m_pNanoscopeStage->writeStage(i + 1, stop, 0);
		});		
	}
}

void QDeviceControlTab::createDpcIlluminationControl()
{
	// Create widgets for dpc illumination control
	m_pGroupBox_DpcIlluminationControl = new QGroupBox;
	QVBoxLayout *pVBoxLayout_DpcIlluminationControl = new QVBoxLayout;
	pVBoxLayout_DpcIlluminationControl->setSpacing(3);
	
	m_pLabel_Illumination = new QLabel(" Illumination", m_pGroupBox_DpcIlluminationControl);
	m_pLabel_Illumination->setDisabled(true);

	m_pRadioButton_Off = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_Off->setText("Off");
	m_pRadioButton_Off->setDisabled(true);
	m_pRadioButton_BrightField = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_BrightField->setText("Brightfield");
	m_pRadioButton_BrightField->setDisabled(true);
	m_pRadioButton_DarkField = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_DarkField->setText("Darkfield");
	m_pRadioButton_DarkField->setDisabled(true);

	m_pRadioButton_Top = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_Top->setText("Top");
	m_pRadioButton_Top->setDisabled(true);
	m_pRadioButton_Bottom = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_Bottom->setText("Bottom");
	m_pRadioButton_Bottom->setDisabled(true);
	m_pRadioButton_Left = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_Left->setText("Left");
	m_pRadioButton_Left->setDisabled(true);
	m_pRadioButton_Right = new QRadioButton(m_pGroupBox_DpcIlluminationControl);
	m_pRadioButton_Right->setText("Right");
	m_pRadioButton_Right->setDisabled(true);
	
	m_pButtonGroup_Illumination = new QButtonGroup(this);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_Off, off);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_BrightField, brightfield);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_DarkField, darkfield);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_Top, illum_top);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_Bottom, illum_bottom);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_Left, illum_left);
	m_pButtonGroup_Illumination->addButton(m_pRadioButton_Right, illum_right);
	m_pRadioButton_Off->setChecked(true);
	

	QGridLayout *pGridLayout_DpcIlluminationControl = new QGridLayout;
	pGridLayout_DpcIlluminationControl->setSpacing(3);

	QHBoxLayout *pHBoxLayout_DpcIllumination1 = new QHBoxLayout;
	pHBoxLayout_DpcIllumination1->setSpacing(3);

	pHBoxLayout_DpcIllumination1->addWidget(m_pLabel_Illumination);
	pHBoxLayout_DpcIllumination1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_DpcIllumination1->addWidget(m_pRadioButton_Off);
	pHBoxLayout_DpcIllumination1->addWidget(m_pRadioButton_BrightField);
	pHBoxLayout_DpcIllumination1->addWidget(m_pRadioButton_DarkField);

	QHBoxLayout *pHBoxLayout_DpcIllumination2 = new QHBoxLayout;
	pHBoxLayout_DpcIllumination2->setSpacing(3);

	pHBoxLayout_DpcIllumination2->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_DpcIllumination2->addWidget(m_pRadioButton_Top);
	pHBoxLayout_DpcIllumination2->addWidget(m_pRadioButton_Bottom);
	pHBoxLayout_DpcIllumination2->addWidget(m_pRadioButton_Left);
	pHBoxLayout_DpcIllumination2->addWidget(m_pRadioButton_Right);
	
	pGridLayout_DpcIlluminationControl->addItem(pHBoxLayout_DpcIllumination1, 0, 0);
	pGridLayout_DpcIlluminationControl->addItem(pHBoxLayout_DpcIllumination2, 1, 0);

	m_pGroupBox_DpcIlluminationControl->setLayout(pGridLayout_DpcIlluminationControl);
	m_pGroupBox_DpcIlluminationControl->setVisible(false);
	
	// Connect signal and slot
	connect(m_pButtonGroup_Illumination, SIGNAL(buttonClicked(int)), this, SLOT(changeIlluminationMode(int)));
}


void QDeviceControlTab::applyPmtGainVoltage(bool toggled)
{
    if (toggled)
    {
        // Set text
        m_pCheckBox_PmtGainControl->setText("Stop Applying PMT Gain Control");

        // Set enabled  for PMT gain control widgets
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
        if (!m_pFlimTrigControlLaser)
        {
			m_pFlimTrigControlLaser = new FlimTrigger;
			m_pFlimTrigControlLaser->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }
		m_pFlimTrigControlLaser->sourceTerminal = NI_FLIM_TRIG_SOURCE;
		m_pFlimTrigControlLaser->counterChannel = NI_FLIM_TRIG_CHANNEL;
		m_pFlimTrigControlLaser->frequency = FLIM_LASER_REP_RATE;
		m_pFlimTrigControlLaser->finite_samps = FLIM_LASER_FINITE_SAMPS;

		if (!m_pFlimTrigControlDAQ)
		{
			m_pFlimTrigControlDAQ = new FlimTrigger;
			m_pFlimTrigControlDAQ->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}
		m_pFlimTrigControlDAQ->sourceTerminal = ALAZAR_DAQ_TRIG_SOURCE;
		m_pFlimTrigControlDAQ->counterChannel = ALAZAR_DAQ_TRIG_CHANNEL;
		m_pFlimTrigControlDAQ->frequency = FLIM_LASER_REP_RATE;
		m_pFlimTrigControlDAQ->finite_samps = FLIM_LASER_FINITE_SAMPS;

		//// Create master triggering control objects
		//if (!m_pPulseTrainMaster)
		//{
		//	m_pPulseTrainMaster = new PulseTrainGenerator;
		//	m_pPulseTrainMaster->SendStatusMessage += [&](const char* msg, bool is_error) {
		//		QString qmsg = QString::fromUtf8(msg);
		//		emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
		//	};
		//}

		//// Initializing
		//m_pPulseTrainMaster->counterChannel = NI_MASTER_TRIG_CHANNEL;
		//m_pPulseTrainMaster->triggerSource = NI_MASTER_TRIG_SOURCE; // Corpus callosum = SY Kwon
		//m_pPulseTrainMaster->frequency = CRS_SCAN_FREQ;
		
        // Initializing
		if (!m_pFlimTrigControlLaser->initialize() || !m_pFlimTrigControlDAQ->initialize()) // || !m_pPulseTrainMaster->initialize())
        {
            m_pCheckBox_FlimLaserTrigControl->setChecked(false);
            return;
        }

        // Generate FLIm laser sync
		m_pFlimTrigControlLaser->start();
		m_pFlimTrigControlDAQ->start();

		//std::this_thread::sleep_for(std::chrono::milliseconds(200));
		//m_pPulseTrainMaster->start();
    }
    else
    {
        // Delete FLIm laser sync control objects
		//if (m_pPulseTrainMaster)
		//{
		//	m_pPulseTrainMaster->stop();
		//	delete m_pPulseTrainMaster;
		//	m_pPulseTrainMaster = nullptr;
		//}

        if (m_pFlimTrigControlLaser)
        {
			m_pFlimTrigControlLaser->stop();
            delete m_pFlimTrigControlLaser;
			m_pFlimTrigControlLaser = nullptr;
        }

		if (m_pFlimTrigControlDAQ)
		{
			m_pFlimTrigControlDAQ->stop();
			delete m_pFlimTrigControlDAQ;
			m_pFlimTrigControlDAQ = nullptr;
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

		//if (m_pStreamTab)
		//{
		//	m_pStreamTab->setYLinesWidgets(false);
		//}


#if (CRS_DIR_FACTOR == 2)
		// Create two edge detection enable control objects
		if (!m_pTwoEdgeTriggerEnable)
		{
			m_pTwoEdgeTriggerEnable = new TwoEdgeTriggerEnable;
			m_pTwoEdgeTriggerEnable->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		m_pTwoEdgeTriggerEnable->lines = NI_GALVO_TWO_EDGE_LINE;
#endif

        // Create Galvano mirror control objects
        if (!m_pGalvoScan)
        {
            m_pGalvoScan = new GalvoScan;
            m_pGalvoScan->SendStatusMessage += [&](const char* msg, bool is_error) {
                QString qmsg = QString::fromUtf8(msg);
                emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
            };
        }

		m_pGalvoScan->sourceTerminal = (FAST_DIR_FACTOR == 1) ? NI_GALVO_SOURCE : NI_GALVO_SOURCE_BIDIR;
		m_pGalvoScan->triggerSource = NI_GALVO_START_TRIG_SOURCE;
        m_pGalvoScan->pp_voltage = m_pLineEdit_PeakToPeakVoltage->text().toDouble();
        m_pGalvoScan->offset = m_pLineEdit_OffsetVoltage->text().toDouble();        
		m_pGalvoScan->step = 1 * (m_pConfig->nLines + GALVO_FLYING_BACK + 2);

        // Initializing
#if (FAST_DIR_FACTOR == 1)
		if (!m_pGalvoScan->initialize())
#elif (FAST_DIR_FACTOR == 2)		
		if (!m_pTwoEdgeTriggerEnable->initialize() || !m_pGalvoScan->initialize())
#endif
		{
			m_pCheckBox_GalvoScanControl->setChecked(false);
			return;
		}
		else
		{
#if (CRS_DIR_FACTOR == 2)
			m_pTwoEdgeTriggerEnable->start();
#endif
			m_pGalvoScan->start();
		}
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

#if (CRS_DIR_FACTOR == 2)
		// Delete two edge detection enable control objects
		if (m_pTwoEdgeTriggerEnable)
		{
			m_pTwoEdgeTriggerEnable->stop();
			delete m_pTwoEdgeTriggerEnable;
			m_pTwoEdgeTriggerEnable = nullptr;
		}
#endif

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


void QDeviceControlTab::connectNanoscopeStage(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_NanoscopeStageControl->setText("Disconnect from Nanoscope Stage");

		// Create Nanoscope stage control objects
		if (!m_pNanoscopeStage)
		{
			m_pNanoscopeStage = new NanoscopeStage;
			m_pNanoscopeStage->SetPortName(NANOSCOPE_STAGE_PORT);

			m_pNanoscopeStage->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');	
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}

		// Set callback function
		m_pNanoscopeStage->DidWriteCommand += [&](bool _moving) {
			m_pStreamTab->m_bIsStageTransition = _moving;
			if (_moving)
			{
				m_pStreamTab->m_bIsStageTransited = false; // ¼ø¿ëÄ® true;
				m_pStreamTab->m_bRecordingPhase = false;
			}
			m_pStreamTab->m_nAverageCount = 1;
		};

		m_pNanoscopeStage->DidGetCurPos += [&](uint8_t stage, double cur_pos) {
			m_pConfig->NanoscopePosition[stage - 1] = (int)(cur_pos * 1'000'000.0);
			m_pLineEdit_Pos[stage - 1]->setText(QString::number(cur_pos, 'f', 4));
		};

		// Connect stage
		if (!(m_pNanoscopeStage->ConnectDevice()))
		{
			m_pCheckBox_NanoscopeStageControl->setChecked(false);
			return;
		}
		else
		{
			for (int i = 0; i < 3; i++)
				m_pGroupBox_Axis[i]->setEnabled(true);			
		}
		
		//m_pNanoscopeStage->DidMovedRelative += [&]() {
		//	std::this_thread::sleep_for(std::chrono::milliseconds(250));
		//	m_pNanoscopeStage_XY->setIsMoving(false);
		//	if (m_pStreamTab->getOperationTab()->m_pMemoryBuffer->m_bIsRecording)
		//		if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
		//		{
		//		}
		//			//getStreamTab()->m_bIsStageTransition = false;  //false

		//	//if (m_pStreamTab->getOperationTab()->m_pMemoryBuffer->m_bIsRecording)
		//	//{
		//	//	if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
		//	//	{					
		//	//		m_pCheckBox_FlimLaserTrigControl->setChecked(true);

		//	//		//std::unique_lock<std::mutex> mlock(m_mtxStageScan);
		//	//		//mlock.unlock();
		//	//		//m_cvStageScan.notify_one();
		//	//	}
		//	//}
		//};
					
		// Set enable true for Nanoscope stage control widgets
		m_pLabel_StageUnit->setEnabled(true);
		
		m_pStreamTab->getImageStitchingCheckBox()->setEnabled(true);

		// signal & slot connection
		connect(this, SIGNAL(startStageScan(int, int)), this, SLOT(stageScan(int, int)));
		connect(this, SIGNAL(monitoring()), this, SLOT(getCurrentPosition()));
	}
	else
	{
		// signal & slot disconnection
		disconnect(this, SIGNAL(startStageScan(int, int)), 0, 0);
		disconnect(this, SIGNAL(monitoring()), 0, 0);

		// Set enable false for Nanoscope stage control widgets
		if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
			m_pStreamTab->getImageStitchingCheckBox()->setChecked(false);
		m_pStreamTab->getImageStitchingCheckBox()->setDisabled(true);

		m_pLabel_StageUnit->setDisabled(true);		
		for (int i = 0; i < 3; i++)	
			m_pGroupBox_Axis[i]->setDisabled(true);

		if (m_pNanoscopeStage)
		{
			// Disconnect the Stage
			m_pNanoscopeStage->DisconnectDevice();

			// Delete Nanoscope stage control objects
			delete m_pNanoscopeStage;
			m_pNanoscopeStage = nullptr;
		}

		// Set text
		m_pCheckBox_NanoscopeStageControl->setText("Connect to Nanoscope Stage");
	}
}

void QDeviceControlTab::stageScan(int dev_num, int length)
{
	//if (length != 0)
	if (length > 0)
		m_pNanoscopeStage->writeStage(dev_num, rel_ccw, length);
	else
		m_pNanoscopeStage->writeStage(dev_num, rel_cw, -length);
}

void QDeviceControlTab::getCurrentPosition()
{
}


void QDeviceControlTab::connectDpcIlluminationControl(bool toggled)
{
	if (toggled)
	{		
		// Create DPC illumination control objects
		if (!m_pDpcIllumControl)
		{
			m_pDpcIllumControl = new DpcIllumination;
			m_pDpcIllumControl->SetPortName(ILLUMINATION_COM_PORT);

			m_pDpcIllumControl->SendStatusMessage += [&](const char* msg, bool is_error) {
				QString qmsg = QString::fromUtf8(msg);
				qmsg.replace('\n', ' ');
				emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
			};
		}
		
		// Connect stage
		if (!(m_pDpcIllumControl->ConnectDevice()))
		{
			connectDpcIlluminationControl(false);
			return;
		}

		// Set enable true for DPC illumination control widgets
		m_pLabel_Illumination->setEnabled(true);
		m_pRadioButton_Off->setEnabled(true);
		m_pRadioButton_BrightField->setEnabled(true);
		m_pRadioButton_DarkField->setEnabled(true);
		m_pRadioButton_Top->setEnabled(true);
		m_pRadioButton_Bottom->setEnabled(true);
		m_pRadioButton_Left->setEnabled(true);
		m_pRadioButton_Right->setEnabled(true);
	}
	else
	{
		// LED off first
		m_pRadioButton_Off->setChecked(true);

		// Set enable false for DPC illumination control widgets
		m_pLabel_Illumination->setEnabled(false);		
		m_pRadioButton_Off->setEnabled(false);
		m_pRadioButton_BrightField->setEnabled(false);
		m_pRadioButton_DarkField->setEnabled(false);
		m_pRadioButton_Top->setEnabled(false);
		m_pRadioButton_Bottom->setEnabled(false);
		m_pRadioButton_Left->setEnabled(false);
		m_pRadioButton_Right->setEnabled(false);
		
		if (m_pDpcIllumControl)
		{
			// Disconnect the com port
			m_pDpcIllumControl->DisconnectDevice();

			// Delete DPC illumination control objects
			delete m_pDpcIllumControl;
			m_pDpcIllumControl = nullptr;
		}
	}

}

void QDeviceControlTab::changeIlluminationMode(int pattern)
{
	m_pDpcIllumControl->setIlluminationPattern(pattern);
}
