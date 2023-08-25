
#include "QStreamTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Dialog/FlimCalibDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>

#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/ImagingSource/ImagingSource.h>

#include <DeviceControl/NanoscopeStage/NanoscopeStage.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <mkl_service.h>
#include <mkl_df.h>


QStreamTab::QStreamTab(QWidget *parent) :
	QDialog(parent), m_nWrittenSamples(0), m_nAcquiredFrames(0), m_nAverageCount(1),
	m_bRecordingPhase(false), m_bIsStageTransition(false), m_bIsStageTransited(false), m_nImageCount(0)
{
	// Set main window objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
	m_pConfig = m_pMainWnd->m_pConfiguration;

	// Create message window
	m_pListWidget_MsgWnd = new QListWidget(this);
	m_pListWidget_MsgWnd->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	m_pListWidget_MsgWnd->setStyleSheet("font: 7pt");
	m_pConfig->msgHandle += [&](const char* msg) {
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, false);
	};
	connect(this, SIGNAL(sendStatusMessage(QString, bool)), this, SLOT(processMessage(QString, bool)));

	// Create image modality widgets
	m_pLabel_Modality = new QLabel(this);
	m_pLabel_Modality->setText("Modality   ");

	m_pRadioButton_FLIM = new QRadioButton(this);
	m_pRadioButton_FLIM->setText("FLIM");
	m_pRadioButton_FLIM->setChecked(true);
	m_pRadioButton_DPC = new QRadioButton(this);
	m_pRadioButton_DPC->setText("Differential Phase Contrast");

	m_pButtonGroup_Modality = new QButtonGroup(this);
	m_pButtonGroup_Modality->addButton(m_pRadioButton_FLIM, MODALITY_FLIM);
	m_pButtonGroup_Modality->addButton(m_pRadioButton_DPC, MODALITY_DPC);

	// Create image size widgets
	m_pLabel_YLines = new QLabel(this);
	m_pLabel_YLines->setText("#Y Lines  ");

	m_pRadioButton_126 = new QRadioButton(this);
	m_pRadioButton_126->setText("126");
	m_pRadioButton_250 = new QRadioButton(this);
	m_pRadioButton_250->setText("250");
	m_pRadioButton_500 = new QRadioButton(this);
	m_pRadioButton_500->setText("500");
	m_pRadioButton_1000 = new QRadioButton(this);
	m_pRadioButton_1000->setText("1000");

	m_pButtonGroup_YLines = new QButtonGroup(this);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_126, N_LINES_126);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_250, N_LINES_250);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_500, N_LINES_500);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_1000, N_LINES_1000);

	switch (m_pConfig->nLines)
	{
	case N_LINES_126:
		m_pRadioButton_126->setChecked(true);
		m_pConfig->nLines = N_LINES_126;
		break;
	case N_LINES_250:
		m_pRadioButton_250->setChecked(true);
		m_pConfig->nLines = N_LINES_250;
		break;
	case N_LINES_500:
		m_pRadioButton_500->setChecked(true);
		m_pConfig->nLines = N_LINES_500;
		break;
	case N_LINES_1000:
		m_pRadioButton_1000->setChecked(true);
		m_pConfig->nLines = N_LINES_1000;
		break;
	default:
		m_pRadioButton_1000->setChecked(true);
		m_pConfig->nLines = N_LINES_1000;
		break;
	}

	// Create averaing widgets
	m_pLabel_Averaging = new QLabel(this);
	m_pLabel_Averaging->setText("Averaging Frame ");

	m_pLineEdit_Averaging = new QLineEdit(this);
	m_pLineEdit_Averaging->setFixedWidth(25);
	m_pLineEdit_Averaging->setText(QString::number(m_pConfig->imageAveragingFrames));
	m_pLineEdit_Averaging->setAlignment(Qt::AlignCenter);
	m_pLineEdit_Averaging->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	m_pLabel_AcquisitionStatus = new QLabel(this);
	QString str; str.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", 0, m_pConfig->imageSize, 0, m_pConfig->imageAveragingFrames, 0, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	m_pLabel_AcquisitionStatus->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_pLabel_AcquisitionStatus->setText(str);

#if (CRS_DIR_FACTOR == 2)
	// Create bidirectional scan compensation-related widgets
	m_pLabel_BiDirScanComp = new QLabel(this);
	m_pLabel_BiDirScanComp->setText("Bidirectional Scan Compensation  ");

	m_pDoubleSpinBox_BiDirScanComp = new QDoubleSpinBox(this);
	m_pDoubleSpinBox_BiDirScanComp->setFixedWidth(50);
	m_pDoubleSpinBox_BiDirScanComp->setRange(-100.0, 100.0);
	m_pDoubleSpinBox_BiDirScanComp->setSingleStep(0.1);
	m_pDoubleSpinBox_BiDirScanComp->setValue(m_pConfig->biDirScanComp);
	m_pDoubleSpinBox_BiDirScanComp->setDecimals(1);
	m_pDoubleSpinBox_BiDirScanComp->setAlignment(Qt::AlignCenter);
	m_pDoubleSpinBox_BiDirScanComp->setDisabled(false);

	m_pPushButton_AutoCorr = new QPushButton(this);
	m_pPushButton_AutoCorr->setText("Auto");	
	m_pPushButton_AutoCorr->setFixedWidth(40);
#endif
		
	// CRS nonlinearity compensation
	m_pCheckBox_CRSNonlinearityComp = new QCheckBox(this);
	m_pCheckBox_CRSNonlinearityComp->setText("CRS Nonlinearity Compensation");
	///m_pCheckBox_CRSNonlinearityComp->setDisabled(true);

	m_pCRSCompIdx = np::FloatArray2(m_pConfig->nPixels, 2);
	for (int i = 0; i < m_pConfig->nPixels; i++)
	{
		m_pCRSCompIdx(i, 0) = i;
		m_pCRSCompIdx(i, 1) = 1;
	}

	m_pCheckBox_CRSNonlinearityComp->setChecked(m_pConfig->crsCompensation);
	if (m_pConfig->crsCompensation) changeCRSNonlinearityComp(true);

	// CMOS properties
	m_pLabel_Gain = new QLabel(this);
	m_pLabel_Gain->setText(QString("Gain: %1 dB  ").arg((double)m_pConfig->cmosGain / 10.0, 2, 'f', 1));
	m_pLabel_Gain->setAlignment(Qt::AlignRight);
	m_pLabel_Gain->setFixedWidth(110);	

	m_pSlider_Gain = new QSlider(this);
	m_pSlider_Gain->setOrientation(Qt::Horizontal);
	m_pSlider_Gain->setRange(0, 480);
	m_pSlider_Gain->setSingleStep(1);
	m_pSlider_Gain->setPageStep(10);	
	m_pSlider_Gain->setValue(m_pConfig->cmosGain);

	m_pLabel_Exposure = new QLabel(this);
	m_pLabel_Exposure->setText(QString("Exposure: %1 msec  ").arg(m_pConfig->cmosExposure));
	m_pLabel_Exposure->setAlignment(Qt::AlignRight);
	m_pLabel_Exposure->setFixedWidth(110);

	m_pSlider_Exposure = new QSlider(this);
	m_pSlider_Exposure->setOrientation(Qt::Horizontal);
	m_pSlider_Exposure->setRange(1, 1000);
	m_pSlider_Exposure->setSingleStep(1);
	m_pSlider_Exposure->setPageStep(10);
	m_pSlider_Exposure->setValue(m_pConfig->cmosExposure);

	// Image stitching mode
	m_pCheckBox_StitchingMode = new QCheckBox(this);
	m_pCheckBox_StitchingMode->setText("Enable Stitching Mode");
	m_pCheckBox_StitchingMode->setDisabled(true);

	m_pLabel_XStep = new QLabel("X Step", this);
	m_pLabel_XStep->setDisabled(true);
	m_pLineEdit_XStep = new QLineEdit(this);
	m_pLineEdit_XStep->setFixedWidth(25);
	m_pLineEdit_XStep->setText(QString::number(m_pConfig->imageStichingXStep));
	m_pLineEdit_XStep->setAlignment(Qt::AlignCenter);
	m_pLineEdit_XStep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_XStep->setDisabled(true);

	m_pLabel_YStep = new QLabel("  Y Step", this);
	m_pLabel_YStep->setDisabled(true);
	m_pLineEdit_YStep = new QLineEdit(this);
	m_pLineEdit_YStep->setFixedWidth(25);
	m_pLineEdit_YStep->setText(QString::number(m_pConfig->imageStichingYStep));
	m_pLineEdit_YStep->setAlignment(Qt::AlignCenter);
	m_pLineEdit_YStep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_YStep->setDisabled(true);


	QHBoxLayout *pHBoxLayout_Modality = new QHBoxLayout;
	pHBoxLayout_Modality->setSpacing(2);

	pHBoxLayout_Modality->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_Modality->addWidget(m_pLabel_Modality);
	pHBoxLayout_Modality->addWidget(m_pRadioButton_FLIM);
	pHBoxLayout_Modality->addWidget(m_pRadioButton_DPC);
	
	QHBoxLayout *pHBoxLayout_YLines = new QHBoxLayout;
	pHBoxLayout_YLines->setSpacing(2);

	pHBoxLayout_YLines->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_YLines->addWidget(m_pLabel_YLines);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_126);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_250);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_500);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_1000);

	QHBoxLayout *pHBoxLayout_Averaging = new QHBoxLayout;
	pHBoxLayout_Averaging->setSpacing(2);

	pHBoxLayout_Averaging->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_Averaging->addWidget(m_pLabel_Averaging);
	pHBoxLayout_Averaging->addWidget(m_pLineEdit_Averaging);

#if (CRS_DIR_FACTOR == 2)
	QHBoxLayout *pHBoxLayout_Bidir = new QHBoxLayout;
	pHBoxLayout_Bidir->setSpacing(2);

	pHBoxLayout_Bidir->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_Bidir->addWidget(m_pLabel_BiDirScanComp);
	pHBoxLayout_Bidir->addWidget(m_pDoubleSpinBox_BiDirScanComp);
	pHBoxLayout_Bidir->addWidget(m_pPushButton_AutoCorr);
#endif

	QHBoxLayout *pHBoxLayout_CRS = new QHBoxLayout;
	pHBoxLayout_CRS->setSpacing(2);

	pHBoxLayout_CRS->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_CRS->addWidget(m_pCheckBox_CRSNonlinearityComp);
	
	QVBoxLayout *pVBoxLayout_ImageSize = new QVBoxLayout;
	pVBoxLayout_ImageSize->setSpacing(2);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_YLines);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_Averaging);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_CRS);	
#if (CRS_DIR_FACTOR == 2)
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_Bidir);
#endif	
	pVBoxLayout_ImageSize->addWidget(m_pLabel_AcquisitionStatus);
	
	QGridLayout *pGridLayout_CmosProperties = new QGridLayout;
	pGridLayout_CmosProperties->setSpacing(2);
	pGridLayout_CmosProperties->addWidget(m_pLabel_Gain, 0, 0);
	pGridLayout_CmosProperties->addWidget(m_pSlider_Gain, 0, 1);
	pGridLayout_CmosProperties->addWidget(m_pLabel_Exposure, 1, 0);
	pGridLayout_CmosProperties->addWidget(m_pSlider_Exposure, 1, 1);

	QGridLayout *pGridLayout_ImageStitching = new QGridLayout;
	pGridLayout_ImageStitching->setSpacing(2);

	pGridLayout_ImageStitching->addWidget(m_pCheckBox_StitchingMode, 0, 0);
	pGridLayout_ImageStitching->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed), 0, 1);
	pGridLayout_ImageStitching->addWidget(m_pLabel_XStep, 0, 2);
	pGridLayout_ImageStitching->addWidget(m_pLineEdit_XStep, 0, 3);
	pGridLayout_ImageStitching->addWidget(m_pLabel_YStep, 0, 4);
	pGridLayout_ImageStitching->addWidget(m_pLineEdit_YStep, 0, 5);


	// Create streaming objects
	m_pOperationTab = new QOperationTab(this);
	m_pDeviceControlTab = new QDeviceControlTab(this);
	m_pVisualizationTab = new QVisualizationTab(m_pConfig, this);

	// Image y lines & averaging initialization
	changeYLines(m_pConfig->nLines);

	// Create group boxes for streaming objects
	m_pGroupBox_OperationTab = new QGroupBox();
	m_pGroupBox_OperationTab->setLayout(m_pOperationTab->getLayout());
	m_pGroupBox_OperationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_OperationTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

	m_pGroupBox_DeviceControlTab = new QGroupBox();
	m_pGroupBox_DeviceControlTab->setLayout(m_pDeviceControlTab->getLayout());
	m_pGroupBox_DeviceControlTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_DeviceControlTab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pGroupBox_DeviceControlTab->setFixedWidth(332);

	m_pGroupBox_VisualizationTab = new QGroupBox();
	m_pGroupBox_VisualizationTab->setLayout(m_pVisualizationTab->getLayout());
	m_pGroupBox_VisualizationTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_VisualizationTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_pGroupBox_ModalityTab = new QGroupBox();
	m_pGroupBox_ModalityTab->setLayout(pHBoxLayout_Modality);
	m_pGroupBox_ModalityTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_ModalityTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pGroupBox_ModalityTab->setFixedWidth(332);

	m_pGroupBox_ImageSizeTab = new QGroupBox();
	m_pGroupBox_ImageSizeTab->setLayout(pVBoxLayout_ImageSize);
	m_pGroupBox_ImageSizeTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_ImageSizeTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pGroupBox_ImageSizeTab->setFixedWidth(332);
	
	m_pGroupBox_CmosProperties = new QGroupBox();
	m_pGroupBox_CmosProperties->setLayout(pGridLayout_CmosProperties);
	m_pGroupBox_CmosProperties->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_CmosProperties->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pGroupBox_CmosProperties->setFixedWidth(312);
	m_pGroupBox_CmosProperties->setVisible(false);

	m_pGroupBox_StitchingTab = new QGroupBox();
	m_pGroupBox_StitchingTab->setLayout(pGridLayout_ImageStitching);
	m_pGroupBox_StitchingTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_StitchingTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pGroupBox_StitchingTab->setFixedWidth(332);

	// Create thread managers for data processing
	m_pThreadFlimProcess = new ThreadManager("FLIm image process");
	m_pThreadVisualization = new ThreadManager("Visualization process");

	// Create buffers for threading operation		
	m_syncFlimProcessing.allocate_queue_buffer(m_pConfig->nScans, m_pConfig->nTimes, PROCESSING_BUFFER_SIZE); // FLIm Processing
	m_syncFlimVisualization.allocate_queue_buffer(11, m_pConfig->nTimes, PROCESSING_BUFFER_SIZE); // FLIm Visualization
	m_syncDpcProcessing.allocate_queue_buffer(CMOS_WIDTH, CMOS_HEIGHT, PROCESSING_BUFFER_SIZE); // DPC Processing 

	// Set signal object
	setFlimAcquisitionCallback();
	setFlimProcessingCallback();
	setDpcAcquisitionCallback();
	setVisualizationCallback();

	// Create layout
	QGridLayout* pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(2);

	// Set layout dpc
	m_pVisualizationTab->getVBoxLayoutVisWidgets()->addWidget(m_pDeviceControlTab->getDpcIlluminationGroupBox());
	m_pVisualizationTab->getVBoxLayoutVisWidgets()->addWidget(m_pGroupBox_CmosProperties);
	m_pVisualizationTab->getVBoxLayoutVisWidgets()->addWidget(m_pVisualizationTab->getDpcVisualizationBox());
	m_pVisualizationTab->getVBoxLayoutVisWidgets()->addWidget(m_pVisualizationTab->getQpiVisualizationBox());
	m_pVisualizationTab->getVisualizationWidgetsBox()->setLayout(m_pVisualizationTab->getVBoxLayoutVisWidgets());

	// Set layout
	pGridLayout->addWidget(m_pGroupBox_VisualizationTab, 0, 0, 7, 1);
	pGridLayout->addWidget(m_pGroupBox_OperationTab, 0, 1);
	pGridLayout->addWidget(m_pGroupBox_ModalityTab, 1, 1);
	pGridLayout->addWidget(m_pVisualizationTab->getVisualizationWidgetsBox(), 2, 1);
	pGridLayout->addWidget(m_pGroupBox_ImageSizeTab, 3, 1);
	pGridLayout->addWidget(m_pGroupBox_StitchingTab, 4, 1);
	pGridLayout->addWidget(m_pGroupBox_DeviceControlTab, 5, 1);
	pGridLayout->addWidget(m_pListWidget_MsgWnd, 6, 1);

	this->setLayout(pGridLayout);

	// Set timer for monitoring status
	m_pTimer_Monitoring = new QTimer(this);
	m_pTimer_Monitoring->start(100); // renew per 100 msec

	// Connect signal and slot
	connect(m_pButtonGroup_Modality, SIGNAL(buttonClicked(int)), this, SLOT(changeModality(int)));
	connect(m_pButtonGroup_YLines, SIGNAL(buttonClicked(int)), this, SLOT(changeYLines(int)));
	connect(m_pLineEdit_Averaging, SIGNAL(textChanged(const QString &)), this, SLOT(changeAveragingFrame(const QString &)));
#if (2 == CRS_DIR_FACTOR)	
		connect(m_pDoubleSpinBox_BiDirScanComp, SIGNAL(valueChanged(double)), this, SLOT(changeBiDirScanComp(double)));
		connect(m_pPushButton_AutoCorr, SIGNAL(clicked(bool)), this, SLOT(autoCrsCompSet()));
#endif
	connect(m_pCheckBox_CRSNonlinearityComp, SIGNAL(toggled(bool)), this, SLOT(changeCRSNonlinearityComp(bool)));
	connect(m_pSlider_Gain, SIGNAL(valueChanged(int)), this, SLOT(changeCmosGain(int)));
	connect(m_pSlider_Exposure, SIGNAL(valueChanged(int)), this, SLOT(changeCmosExposure(int)));
	connect(m_pCheckBox_StitchingMode, SIGNAL(toggled(bool)), this, SLOT(enableStitchingMode(bool)));
	connect(this, SIGNAL(setAcquisitionStatus(QString)), m_pLabel_AcquisitionStatus, SLOT(setText(QString)));
	connect(m_pLineEdit_XStep, SIGNAL(textChanged(const QString &)), this, SLOT(changeStitchingXStep(const QString &)));
	connect(m_pLineEdit_YStep, SIGNAL(textChanged(const QString &)), this, SLOT(changeStitchingYStep(const QString &)));
	connect(m_pTimer_Monitoring, SIGNAL(timeout()), this, SLOT(onTimerMonitoring()));
}

QStreamTab::~QStreamTab()
{
	m_pTimer_Monitoring->stop();
    if (m_pThreadVisualization) delete m_pThreadVisualization;	
    if (m_pThreadFlimProcess) delete m_pThreadFlimProcess;
}

void QStreamTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}

void QStreamTab::setWidgetsText()
{
}

void QStreamTab::setYLinesWidgets(bool enabled)
{
	m_nWrittenSamples = 0;
	m_nAcquiredFrames = 0;
	m_nImageCount = 0;
	m_nAverageCount = 1;

	m_pLabel_Modality->setEnabled(enabled);
	m_pRadioButton_FLIM->setEnabled(enabled);
	m_pRadioButton_DPC->setEnabled(enabled);

    m_pLabel_YLines->setEnabled(enabled);
	m_pRadioButton_126->setEnabled(enabled);
    m_pRadioButton_250->setEnabled(enabled);
    m_pRadioButton_500->setEnabled(enabled);
    m_pRadioButton_1000->setEnabled(enabled);
}

void QStreamTab::setAveragingWidgets(bool enabled)
{
	m_pLabel_Averaging->setEnabled(enabled);
	m_pLineEdit_Averaging->setEnabled(enabled);
	m_pCheckBox_CRSNonlinearityComp->setEnabled(enabled);

	if (getDeviceControlTab()->getNanoscopeStageControl()->isChecked())
	{
		m_pCheckBox_StitchingMode->setEnabled(enabled);
		if (m_pCheckBox_StitchingMode->isChecked())
		{
			m_pLabel_XStep->setEnabled(enabled);
			m_pLineEdit_XStep->setEnabled(enabled);
			m_pLabel_YStep->setEnabled(enabled);
			m_pLineEdit_YStep->setEnabled(enabled);
		}
	}

	if (!enabled)
	{
		m_bIsStageTransition = false;
		m_bIsStageTransited = false; // 순용이가 칼꼽음, 원래 true;
		m_nImageCount = 0;
	}
}

void QStreamTab::updateCmosGainExposure()
{
	ImagingSource *pImagingSource = m_pOperationTab->getDataAcq()->getImagingSource();
	if (pImagingSource)
	{
		while (1) 
		{
			if (pImagingSource->isLive())
			{
				pImagingSource->setGain((double)m_pConfig->cmosGain / 10.0);
				pImagingSource->setExposure((double)m_pConfig->cmosExposure / 1000.0);
				break;
			}
		}
	}
}

void QStreamTab::setFlimAcquisitionCallback()
{
	MemoryBuffer *pMemBuff = m_pOperationTab->m_pMemoryBuffer;

	DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
	pDataAcq->ConnectDaqAcquiredFlimData([&, pMemBuff](int frame_count, const void* _frame_ptr) {

		static bool recording_phase = false;
		
		if ((frame_count % (FAST_TOTAL_PIECES / FAST_DIR_FACTOR)) < FAST_VALID_PIECES)  // Pass only first third piece (CRS valid region)
		{
			// Data transfer for FLIm processing
			const uint16_t* frame_ptr = (uint16_t*)_frame_ptr;

			// Get buffer from threading queue
			float* pulse_ptr = nullptr;
			{
				std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);

				if (!m_syncFlimProcessing.queue_buffer.empty())
				{
					pulse_ptr = m_syncFlimProcessing.queue_buffer.front();
					m_syncFlimProcessing.queue_buffer.pop();
				}
			}

			if (pulse_ptr != nullptr)
			{
				// Body			
				ippsConvert_16u32f(frame_ptr, pulse_ptr, m_pConfig->bufferSize);

				// Show pulse
				int frame_count0 = frame_count % (FAST_TOTAL_PIECES / FAST_DIR_FACTOR * (m_pConfig->nLines + GALVO_FLYING_BACK + 2));
				{
					if (m_pDeviceControlTab->getFlimCalibDlg())
					{
						int x, y;
						m_pVisualizationTab->getPixelPos(&x, &y);

						///printf("(%d %d) (%d %d) (%d %d)\n", frame_count, frame_count0, x, y, valid_buf, x0);

						int x0 = !(y % FAST_DIR_FACTOR) ? x : m_pConfig->nPixels - 1 - x;
						if (frame_count0 % (FAST_TOTAL_PIECES / FAST_DIR_FACTOR) == x0 / N_TIMES)
							if (frame_count0 / (FAST_TOTAL_PIECES / FAST_DIR_FACTOR) == (y + GALVO_FLYING_BACK + 2))
								emit m_pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pulse_ptr, x0 % N_TIMES);
					}
				}

				// Copy pulse data to writing buffer
				if ((m_pConfig->imageAveragingFrames == 1) && !m_pCheckBox_StitchingMode->isChecked())
				{
					int frame_count1 = ((frame_count % FAST_TOTAL_PIECES) + FAST_VALID_PIECES * (frame_count / FAST_TOTAL_PIECES)) % pMemBuff->m_vectorWritingPulseBuffer.size();

					if (pMemBuff->m_bIsRecording && (frame_count1 == 0))
						recording_phase = true;

					if (recording_phase)
						memcpy(pMemBuff->m_vectorWritingPulseBuffer.at(frame_count1), frame_ptr, sizeof(uint16_t) * m_pConfig->nScans * m_pConfig->nTimes);

					if (pMemBuff->m_bIsRecording && (frame_count1 == pMemBuff->m_vectorWritingPulseBuffer.size() - 1))
						recording_phase = false;
				}

				// Push the buffer to sync Queue
				m_syncFlimProcessing.Queue_sync.push(pulse_ptr);
			}
		}
    });

    pDataAcq->ConnectDaqStopFlimData([&]() {
		m_syncFlimProcessing.Queue_sync.push(nullptr);
    });

    pDataAcq->ConnectDaqSendStatusMessage([&](const char * msg, bool is_error) {
        if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
        emit sendStatusMessage(qmsg, is_error);
    });
}

void QStreamTab::setFlimProcessingCallback()
{
    // FLIm Process Signal Objects /////////////////////////////////////////////////////////////////////////////////////////
    FLImProcess *pFLIm = m_pOperationTab->getDataAcq()->getFLIm();
    m_pThreadFlimProcess->DidAcquireData += [&, pFLIm] (int frame_count) {

        // Get the buffer from the previous sync Queue
        float* pulse_data = m_syncFlimProcessing.Queue_sync.pop();
        if (pulse_data != nullptr)
        {
            // Get buffers from threading queues
            float* flim_ptr = nullptr;
            {
                std::unique_lock<std::mutex> lock(m_syncFlimVisualization.mtx);

                if (!m_syncFlimVisualization.queue_buffer.empty())
                {
                    flim_ptr = m_syncFlimVisualization.queue_buffer.front();
                    m_syncFlimVisualization.queue_buffer.pop();
                }
            }

            if (flim_ptr != nullptr)
            {
				// FLIm processing
				np::FloatArray2 pulse(pulse_data, m_pConfig->nScans, m_pConfig->nTimes);

                np::FloatArray2 intensity (flim_ptr + 0 * m_pConfig->nTimes, m_pConfig->nTimes, 4);
                np::FloatArray2 mean_delay(flim_ptr + 4 * m_pConfig->nTimes, m_pConfig->nTimes, 4);
                np::FloatArray2 lifetime  (flim_ptr + 8 * m_pConfig->nTimes, m_pConfig->nTimes, 3);
								
				(*pFLIm)(intensity, mean_delay, lifetime, pulse);

                // Push the buffers to sync Queues
                m_syncFlimVisualization.Queue_sync.push(flim_ptr);

                // Return (push) the buffer to the previous threading queue
                {
                    std::unique_lock<std::mutex> lock(m_syncFlimProcessing.mtx);
                    m_syncFlimProcessing.queue_buffer.push(pulse_data);
                }
            }
        }
        else
            m_pThreadFlimProcess->_running = false;

        (void)frame_count;
    };

    m_pThreadFlimProcess->DidStopData += [&]() {
        m_syncFlimVisualization.Queue_sync.push(nullptr);
    };

    m_pThreadFlimProcess->SendStatusMessage += [&](const char* msg, bool is_error) {
        if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
        emit sendStatusMessage(qmsg, is_error);
    };
}

void QStreamTab::setDpcAcquisitionCallback()
{
	DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
	pDataAcq->ConnectBrightfieldAcquiredFlimData([&](int frame_count, const np::Uint16Array2 & frame) {

		// Data transfer for FLIm processing
		const uint16_t* frame_ptr = (uint16_t*)frame.raw_ptr();

		// Get buffer from threading queue
		uint16_t* image_ptr = nullptr;
		{
			std::unique_lock<std::mutex> lock(m_syncDpcProcessing.mtx);

			if (!m_syncDpcProcessing.queue_buffer.empty())
			{
				image_ptr = m_syncDpcProcessing.queue_buffer.front();
				m_syncDpcProcessing.queue_buffer.pop();
			}
		}

		if (image_ptr != nullptr)
		{
			// Body		
			memcpy(image_ptr, frame_ptr, frame.length() * sizeof(uint16_t));
			image_ptr[0] = (uint16_t)((double)frame_count / 10.0) * 10 + frame_count;
				
			// Push the buffer to sync Queue
			m_syncDpcProcessing.Queue_sync.push(image_ptr);
		}
	});

	pDataAcq->ConnectBrightfieldStopFlimData([&]() {
		m_syncDpcProcessing.Queue_sync.push(nullptr);
	});

	pDataAcq->ConnectBrightfieldSendStatusMessage([&](const char * msg, bool is_error) {
		if (is_error) m_pOperationTab->setAcquisitionButton(false);
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, is_error);
	});
}

void QStreamTab::setVisualizationCallback()
{
    // Visualization Signal Objects ///////////////////////////////////////////////////////////////////////////////////////////
    m_pThreadVisualization->DidAcquireData += [&] (int frame_count) {

		MemoryBuffer *pMemBuff = m_pOperationTab->m_pMemoryBuffer;

		if (getCurrentModality())
		{
			// Get the buffers from the previous sync Queues
			float* flim_data = m_syncFlimVisualization.Queue_sync.pop();
			if (flim_data != nullptr)
			{
				// Body
				if (m_pOperationTab->isAcquisitionButtonToggled()) // Only valid if acquisition is running 
				{
					// Effective lines
					int effective_lines = GALVO_FLYING_BACK + m_pConfig->nLines + 2;
					
					// Averaging buffer
					if ((m_nAverageCount == 1) && (m_nWrittenSamples == 0))
					{
						m_pTempIntensity = np::FloatArray2(m_pConfig->nPixels, 3 * effective_lines);
						memset(m_pTempIntensity, 0, sizeof(float) * m_pTempIntensity.length());
						m_pTempLifetime = np::FloatArray2(m_pConfig->nPixels, 3 * effective_lines);
						memset(m_pTempLifetime, 0, sizeof(float) * m_pTempLifetime.length());
						m_pNonNaNIndex = np::FloatArray2(m_pConfig->nPixels, 3 * effective_lines); //nPixels = 1000 , effective_lines = 1000 x 4 + galvo flying back 
						ippsSet_32f(0.0f, m_pNonNaNIndex, m_pNonNaNIndex.length());

						//// Prevent mid-phase recording  (첫 장 버리기)
						//if (pMemBuff->m_bIsRecording && !m_bIsStageTransited)
						//	m_bRecordingPhase = true;
					}

					// Data copy				
					np::FloatArray2 intensity(flim_data + 0 * m_pConfig->nTimes, m_pConfig->nTimes, 4);
					np::FloatArray2 lifetime(flim_data + 8 * m_pConfig->nTimes, m_pConfig->nTimes, 3);
					tbb::parallel_for(tbb::blocked_range<size_t>(0, 3),
						[&](const tbb::blocked_range<size_t>& r) {
						for (size_t i = r.begin(); i != r.end(); ++i)
						{
							ippsAdd_32f_I(&intensity(0, i + 1), &m_pTempIntensity(0, i * effective_lines) + m_nWrittenSamples, m_pConfig->nTimes);
							ippsAdd_32f_I(&lifetime(0, i), &m_pTempLifetime(0, i * effective_lines) + m_nWrittenSamples, m_pConfig->nTimes);

							for (int j = 0; j < m_pConfig->nTimes; j++)
								if (intensity(j, i + 1) > INTENSITY_THRES)
									*(&m_pNonNaNIndex(0, i * effective_lines) + m_nWrittenSamples + j) = *(&m_pNonNaNIndex(0, i * effective_lines) + m_nWrittenSamples + j) + 1.0f;
						}
					});
					m_nWrittenSamples += m_pConfig->nTimes;

					// Update Status
					QString str; str.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", m_nWrittenSamples, m_pConfig->imageSize, m_nAverageCount - 1, m_pConfig->imageAveragingFrames, m_nImageCount, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
					emit setAcquisitionStatus(str);

					if (m_nWrittenSamples == (m_pConfig->imageSize + (GALVO_FLYING_BACK + 2) * m_pConfig->nPixels))
					{
						// Averaging
						tbb::parallel_for(tbb::blocked_range<size_t>(0, 3),
							[&](const tbb::blocked_range<size_t>& r) {
							for (size_t i = r.begin(); i != r.end(); ++i)
							{
								{
									///ippsDivC_32f(&m_pTempIntensity(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_nAverageCount, m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), m_pConfig->imageSize);
									///ippsDivC_32f(&m_pTempLifetime(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_nAverageCount, m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(), m_pConfig->imageSize);

									///ippsMul_32f_I(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), m_pConfig->imageSize);
									///ippsMul_32f_I(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(), m_pConfig->imageSize);
								}
								{
									ippsDiv_32f(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), &m_pTempIntensity(0, i * effective_lines + GALVO_FLYING_BACK + 2),
										m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), m_pConfig->imageSize);
									ippsDiv_32f(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), &m_pTempLifetime(0, i * effective_lines + GALVO_FLYING_BACK + 2),
										m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(), m_pConfig->imageSize);
								}
							}
						});
						m_nAverageCount++;

						// Bi-directional mirroring
						if (FAST_DIR_FACTOR == 2)
						{
							tbb::parallel_for(tbb::blocked_range<size_t>(0, 3),
								[&](const tbb::blocked_range<size_t>& r) {
								for (size_t i = r.begin(); i != r.end(); ++i)								
								{
									// Mirroring
									ippiMirror_32f_C1IR(&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels, { m_pConfig->nPixels, m_pConfig->nLines / 2 }, ippAxsVertical);
									ippiMirror_32f_C1IR(&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels, { m_pConfig->nPixels, m_pConfig->nLines / 2 }, ippAxsVertical);
	
									// Offset compensation
									np::FloatArray2 left(m_pConfig->nPixels, m_pConfig->nLines / 2);
									np::FloatArray2 right(m_pConfig->nPixels, m_pConfig->nLines / 2);

									int shift = int(m_pConfig->biDirScanComp);
									float comp = m_pConfig->biDirScanComp - shift;

									if (m_pConfig->biDirScanComp >= 0)
									{
										// Intensity
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&left(shift, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift, m_pConfig->nLines / 2 });
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&right(shift + 1, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift - 1, m_pConfig->nLines / 2 });

										ippsMulC_32f_I(1.0f - comp, left, left.length());
										ippsMulC_32f_I(comp, right, right.length());

										ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
											&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											{ m_pConfig->nPixels, m_pConfig->nLines / 2 });

										// Lifetime
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&left(shift, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift, m_pConfig->nLines / 2 });
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&right(shift + 1, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift - 1, m_pConfig->nLines / 2 });

										ippsMulC_32f_I(1.0f - comp, left, left.length());
										ippsMulC_32f_I(comp, right, right.length());

										ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
											&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											{ m_pConfig->nPixels, m_pConfig->nLines / 2 });
									}
									else
									{
										// Intensity
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(-shift, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&left(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift, m_pConfig->nLines / 2 });
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(-shift + 1, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&right(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift - 1, m_pConfig->nLines / 2 });

										ippsMulC_32f_I(1.0f + comp, left, left.length());
										ippsMulC_32f_I(-comp, right, right.length());

										ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
											&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											{ m_pConfig->nPixels, m_pConfig->nLines / 2 });

										// Lifetime
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(-shift, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&left(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift, m_pConfig->nLines / 2 });
										ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(-shift + 1, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											&right(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift - 1, m_pConfig->nLines / 2 });

										ippsMulC_32f_I(1.0f + comp, left, left.length());
										ippsMulC_32f_I(-comp, right, right.length());

										ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
											&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * FAST_DIR_FACTOR * m_pConfig->nPixels,
											{ m_pConfig->nPixels, m_pConfig->nLines / 2 });
									}
								}
							});
						}

						// CRS nonlinearity compensation
						if (m_pCheckBox_CRSNonlinearityComp->isChecked())
						{
							np::FloatArray comp_index(&m_pCRSCompIdx(0, 0), m_pConfig->nPixels);
							np::FloatArray comp_weight(&m_pCRSCompIdx(0, 1), m_pConfig->nPixels);

							np::FloatArray2 scanArray0(m_pConfig->nPixels, m_pConfig->nLines);
							np::FloatArray2 scanArray1(m_pConfig->nPixels, m_pConfig->nLines);

							for (int i = 0; i < 3; i++)
							{
								// Intensity
								memcpy(scanArray0, m_pVisualizationTab->m_vecVisIntensity.at(i), sizeof(float) * m_pConfig->nPixels * m_pConfig->nLines);
								
								tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)m_pConfig->nLines),
									[&](const tbb::blocked_range<size_t>& r) {
									for (size_t j = r.begin(); j != r.end(); ++j)
									{
										float* fast_scan = &scanArray0(0, (int)j);
										float* comp_scan = &scanArray1(0, (int)j);

										for (int k = 0; k < m_pConfig->nPixels; k++)
										{
											if (k != m_pConfig->nPixels - 1)
												comp_scan[k] = comp_weight[k] * fast_scan[(int)comp_index[k]] + (1 - comp_weight[k]) * fast_scan[(int)comp_index[k] + 1];
											else
												comp_scan[k] = fast_scan[k];
										}
									}
								});

								memcpy(m_pVisualizationTab->m_vecVisIntensity.at(i), scanArray1, sizeof(float) * m_pConfig->nPixels * m_pConfig->nLines);

								// Lifetime
								memcpy(scanArray0, m_pVisualizationTab->m_vecVisLifetime.at(i), sizeof(float) * m_pConfig->nPixels * m_pConfig->nLines);

								tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)m_pConfig->nLines),
									[&](const tbb::blocked_range<size_t>& r) {
									for (size_t j = r.begin(); j != r.end(); ++j)
									{
										float* fast_scan = &scanArray0(0, (int)j);
										float* comp_scan = &scanArray1(0, (int)j);

										for (int k = 0; k < m_pConfig->nPixels; k++)
										{
											if (k != m_pConfig->nPixels - 1)
												comp_scan[k] = comp_weight[k] * fast_scan[(int)comp_index[k]] + (1 - comp_weight[k]) * fast_scan[(int)comp_index[k] + 1];
											else
												comp_scan[k] = fast_scan[k];
										}
									}
								});

								memcpy(m_pVisualizationTab->m_vecVisLifetime.at(i), scanArray1, sizeof(float) * m_pConfig->nPixels * m_pConfig->nLines);
							}
						}

						// Draw Images
						emit m_pVisualizationTab->drawImage(getCurrentModality());

						// Draw histogram statistics
						if (m_pDeviceControlTab->getFlimCalibDlg())
							emit m_pDeviceControlTab->getFlimCalibDlg()->plotHistogram(m_pVisualizationTab->m_vecVisIntensity.at(m_pConfig->flimEmissionChannel - 1),
								m_pVisualizationTab->m_vecVisLifetime.at(m_pConfig->flimEmissionChannel - 1));
						
						// Recording
						if (!m_bIsStageTransition && !m_bIsStageTransited)
						{
							if (m_nAverageCount > m_pConfig->imageAveragingFrames)
							{
								if (pMemBuff->m_bIsRecording) //m_bRecordingPhase) <- 한장 버리기
								{
									int n_total_images = m_pCheckBox_StitchingMode->isChecked() ? m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep : 1;

									// Get buffer from writing queue
									float* image_ptr = pMemBuff->m_vectorWritingImageBuffer.at(pMemBuff->m_nRecordedFrame);

									if (image_ptr != nullptr)
									{
										///if (!m_bIsGalvoOn)
										///{
										///if (0 == ((m_nImageCount + 1) / m_pConfig->imageStichingXStep + 1) % 2)
										///	for (int i = 0; i < 3; i++)
										///		ippiMirror_32f_C1IR(m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), sizeof(float)* m_pConfig->nPixels, { m_pConfig->nPixels, m_pConfig->nLines }, ippAxsHorizontal);
										///}

										// Body (Copying the frame data)
										for (int i = 0; i < 3; i++)
										{
											memcpy(image_ptr + i * m_pVisualizationTab->m_vecVisIntensity.at(i).length(), m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(),
												sizeof(float) * m_pVisualizationTab->m_vecVisIntensity.at(i).length());
											memcpy(image_ptr + (i + 3) * m_pVisualizationTab->m_vecVisLifetime.at(i).length(), m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(),
												sizeof(float) * m_pVisualizationTab->m_vecVisLifetime.at(i).length());
										}

										pMemBuff->increaseRecordedFrame();
									}

									// Stage scanning for stitching
									if (++m_nImageCount < n_total_images)
									{										
										if (m_nImageCount % m_pConfig->imageStichingXStep != 0) // x move
										{
											int step = ((m_nImageCount / m_pConfig->imageStichingXStep + 1) % 2 ? -1 : +1) * m_pConfig->NanoscopeStep[1];
											emit getDeviceControlTab()->startStageScan(2, step);
										}
										else // y move
										{
											emit getDeviceControlTab()->startStageScan(1, m_pConfig->NanoscopeStep[0]);
										}
									}

									// Finish recording when the buffer is full
									else
									{
										m_nImageCount = 0;

										pMemBuff->setIsRecorded(true);
										pMemBuff->setIsRecording(true);
										m_pOperationTab->setRecordingButton(false);

										if (m_pCheckBox_StitchingMode->isChecked())
										{
											emit getDeviceControlTab()->startStageScan(2, (m_pConfig->imageStichingYStep % 2 == 0 ? 0 : m_pConfig->imageStichingXStep - 1) * m_pConfig->NanoscopeStep[1]);
											emit getDeviceControlTab()->startStageScan(1, -(m_pConfig->imageStichingYStep - 1) * m_pConfig->NanoscopeStep[0]);
										}
									}

									// Reset flag
									//m_bRecordingPhase = false;
								}

								m_nAverageCount = 1;
							}
						}
						else
						{
							m_nAverageCount = 1;
							m_bIsStageTransited = false;
						}

						// Re-initializing
						m_nWrittenSamples = 0;
						m_nAcquiredFrames++;
					}
				}

				// Return (push) the buffer to the previous threading queue
				{
					std::unique_lock<std::mutex> lock(m_syncFlimVisualization.mtx);
					m_syncFlimVisualization.queue_buffer.push(flim_data);
				}
			}
			else
				m_pThreadVisualization->_running = false;
		}
		else
		{
			uint16_t* image_data = m_syncDpcProcessing.Queue_sync.pop();
			if (image_data != nullptr)
			{
				// Body
				if (m_pOperationTab->isAcquisitionButtonToggled()) // Only valid if acquisition is running 
				{
					// Live CMOS mode
					if (m_pVisualizationTab->getCurrentDpcImageMode() == DPC_LIVE)
					{
						// Data copy			
						ippiConvert_16u32f_C1R(image_data, sizeof(uint16_t) * CMOS_WIDTH,
							m_pVisualizationTab->m_liveIntensity, sizeof(float) * CMOS_WIDTH, { CMOS_WIDTH, CMOS_HEIGHT });

						// Draw Images
						emit m_pVisualizationTab->drawImage(getCurrentModality());
					}
					else if (m_pVisualizationTab->getCurrentDpcImageMode() == DPC_PROCESSED)
					{
						// Data copy			
						int pattern = image_data[0] - (uint16_t)((double)image_data[0] / 10.0) * 10;
						ippiConvert_16u32f_C1R(image_data, sizeof(uint16_t) * CMOS_WIDTH,
							m_pVisualizationTab->m_vecIllumImages.at(pattern), sizeof(float) * CMOS_WIDTH, { CMOS_WIDTH, CMOS_HEIGHT });

						// Draw Images
						if (frame_count % 4 == 3)
						{
							emit m_pVisualizationTab->drawImage(getCurrentModality());

							// Recording
							if (pMemBuff->m_bIsRecording)
							{
								// Get buffer from writing queue
								float* image_ptr = pMemBuff->m_vectorWritingImageBuffer.at(pMemBuff->m_nRecordedFrame);

								if (image_ptr != nullptr)
								{
									// Body (Copying the frame data)
									pMemBuff->dpc_mode = m_pVisualizationTab->getCurrentDpcImageMode();
									pMemBuff->dpc_illum = 0;

									if (m_pVisualizationTab->getCurrentDpcImageMode() == DPC_LIVE)
									{
										memcpy(image_ptr, m_pVisualizationTab->m_liveIntensity.raw_ptr(), sizeof(float) * m_pVisualizationTab->m_liveIntensity.length());
										pMemBuff->dpc_illum = m_pDeviceControlTab->getDpcIllumPattern();
									}
									else if (m_pVisualizationTab->getCurrentDpcImageMode() == DPC_PROCESSED)
									{
										for (int i = 0; i < 4; i++)
										{
											memcpy(image_ptr + i * m_pVisualizationTab->m_vecIllumImages.at(i).length(), m_pVisualizationTab->m_vecIllumImages.at(i).raw_ptr(),
												sizeof(float) * m_pVisualizationTab->m_vecIllumImages.at(i).length());
										}
									}

									pMemBuff->increaseRecordedFrame();

									// Finish recording when the buffer is full					
									pMemBuff->setIsRecorded(true);
									pMemBuff->setIsRecording(true);
									m_pOperationTab->setRecordingButton(false);
								}
							}
						}
					}									
				}

				// Return (push) the buffer to the previous threading queue
				{
					std::unique_lock<std::mutex> lock(m_syncDpcProcessing.mtx);
					m_syncDpcProcessing.queue_buffer.push(image_data);
				}
			}
			else
				m_pThreadVisualization->_running = false;
		}
	};

    m_pThreadVisualization->DidStopData += [&]() {
        // None
    };

    m_pThreadVisualization->SendStatusMessage += [&](const char* msg, bool is_error) {
        if (is_error) m_pOperationTab->setAcquisitionButton(false);
        QString qmsg = QString::fromUtf8(msg);
        emit sendStatusMessage(qmsg, is_error);
    };
}


void QStreamTab::onTimerMonitoring()
{
	if (getOperationTab()->isAcquisitionButtonToggled())
	{
		m_pMainWnd->m_pStatusLabel_Acquisition->setText("Acquistion O");
		m_pMainWnd->m_pStatusLabel_Acquisition->setStyleSheet("color: green;");
	}
	else
	{
		m_pMainWnd->m_pStatusLabel_Acquisition->setText("Acquistion X");
		m_pMainWnd->m_pStatusLabel_Acquisition->setStyleSheet("color: red;");
	}
	
	m_bIsStageTransition = getDeviceControlTab()->getNanoscopeStage() && getDeviceControlTab()->getNanoscopeStage()->getIsMoving();
	if (m_bIsStageTransition)
	{
		m_pMainWnd->m_pStatusLabel_StageMoving->setText("Stage Moving O");
		m_pMainWnd->m_pStatusLabel_StageMoving->setStyleSheet("color: green;");
	}
	else
	{
		m_pMainWnd->m_pStatusLabel_StageMoving->setText("Stage Moving X");
		m_pMainWnd->m_pStatusLabel_StageMoving->setStyleSheet("color: red;");
	}

	if (getOperationTab()->m_pMemoryBuffer->m_bIsRecording && !m_bIsStageTransition) // && getOperationTab()->m_pMemoryBuffer->m_bIsFirstRecImage)
	{
		m_pMainWnd->m_pStatusLabel_Recording->setText("Recording O");
		m_pMainWnd->m_pStatusLabel_Recording->setStyleSheet("color: green;");
	}
	else
	{
		m_pMainWnd->m_pStatusLabel_Recording->setText("Recording X");
		m_pMainWnd->m_pStatusLabel_Recording->setStyleSheet("color: red;");
	}
}


void QStreamTab::changeModality(int modality)
{
	if (modality == MODALITY_FLIM)
	{
		m_pVisualizationTab->getRadioButtonLive()->setChecked(true);
		m_pVisualizationTab->setDpcImageMode(DPC_LIVE);
		m_pVisualizationTab->getFlimVisualizationBox()->setEnabled(true);
		m_pVisualizationTab->getFlimVisualizationBox()->setVisible(true);		
		m_pVisualizationTab->getDpcImageModeBox()->setDisabled(true);
		m_pVisualizationTab->getDpcImageModeBox()->setVisible(false);
		m_pDeviceControlTab->getDpcIlluminationGroupBox()->setDisabled(true);
		m_pDeviceControlTab->getDpcIlluminationGroupBox()->setVisible(false);
		m_pGroupBox_CmosProperties->setDisabled(true);
		m_pGroupBox_CmosProperties->setVisible(false);
		m_pVisualizationTab->getDpcVisualizationBox()->setDisabled(true);
		m_pVisualizationTab->getDpcVisualizationBox()->setVisible(false);
		m_pVisualizationTab->getQpiVisualizationBox()->setDisabled(true);
		m_pVisualizationTab->getQpiVisualizationBox()->setVisible(false);
		m_pGroupBox_ImageSizeTab->setEnabled(true);
		m_pGroupBox_ImageSizeTab->setVisible(true);
		m_pGroupBox_StitchingTab->setEnabled(true);
		m_pDeviceControlTab->getFlimControlGroupBox()->setEnabled(true);
		m_pDeviceControlTab->getScannerControlGroupBox()->setEnabled(true);

		//if (!m_pDeviceControlTab->getResonantScanControl()->isChecked())
		//{
		//	m_pDeviceControlTab->getResonantScanControl()->setChecked(true);
		//	//m_pDeviceControlTab->getResonantScanVoltageControl()->setChecked(true);
		//}
		m_pDeviceControlTab->connectDpcIllumination(false);
	}
	else if (modality == MODALITY_DPC)
	{
		m_pVisualizationTab->getFlimVisualizationBox()->setDisabled(true);
		m_pVisualizationTab->getFlimVisualizationBox()->setVisible(false);
		m_pVisualizationTab->getDpcImageModeBox()->setEnabled(true);
		m_pVisualizationTab->getDpcImageModeBox()->setVisible(true);
		m_pDeviceControlTab->getDpcIlluminationGroupBox()->setEnabled(true);
		m_pDeviceControlTab->getDpcIlluminationGroupBox()->setVisible(true);
		m_pGroupBox_CmosProperties->setEnabled(true);
		m_pGroupBox_CmosProperties->setVisible(true);
		m_pVisualizationTab->getDpcVisualizationBox()->setEnabled(true);
		m_pVisualizationTab->getDpcVisualizationBox()->setVisible(true);
		m_pVisualizationTab->getQpiVisualizationBox()->setEnabled(true);
		m_pVisualizationTab->getQpiVisualizationBox()->setVisible(true);
		m_pGroupBox_ImageSizeTab->setDisabled(true);
		m_pGroupBox_ImageSizeTab->setVisible(false);
		m_pGroupBox_StitchingTab->setDisabled(true);
		m_pDeviceControlTab->getFlimControlGroupBox()->setDisabled(true);
		m_pDeviceControlTab->getScannerControlGroupBox()->setDisabled(true);

		//m_pDeviceControlTab->getResonantScanControl()->setChecked(false);
		m_pDeviceControlTab->connectDpcIllumination(true);
	}

	m_pVisualizationTab->setObjects(m_pConfig->nLines, getCurrentModality());
}

void QStreamTab::changeYLines(int n_lines)
{
	m_pConfig->nLines = n_lines;
	m_pConfig->imageSize = m_pConfig->nPixels * m_pConfig->nLines;
    m_pVisualizationTab->setObjects(m_pConfig->nLines, getCurrentModality());

	m_pOperationTab->getSaveButton()->setEnabled(false);
	m_pOperationTab->getMemBuff()->m_bIsRecorded = false;

	///m_pOperationTab->m_pMemoryBuffer->allocateWritingBuffer();

	QString str; str.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", 0, m_pConfig->imageSize, 0, m_pConfig->imageAveragingFrames, 0, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	m_pLabel_AcquisitionStatus->setText(str);
}

void QStreamTab::changeAveragingFrame(const QString &str)
{
	m_pConfig->imageAveragingFrames = str.toInt();

	QString str1; str1.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", 0, m_pConfig->imageSize, 0, m_pConfig->imageAveragingFrames, 0, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	m_pLabel_AcquisitionStatus->setText(str1);
}

void QStreamTab::changeBiDirScanComp(double comp)
{
	m_pConfig->biDirScanComp = (float)comp;
}

void QStreamTab::autoCrsCompSet()
{
#if FAST_DIR_FACTOR == 2
	//std::thread auto_comp([&]()
	//{
		// Set zero first
		m_pDoubleSpinBox_BiDirScanComp->setValue(0.0);

		std::this_thread::sleep_for(std::chrono::milliseconds(m_pConfig->nLines));

		// Create buffer
		np::FloatArray2 frame(m_pConfig->nPixels, m_pConfig->nLines);
		ippsConvert_8u32f(m_pVisualizationTab->getImgObjIntensity()->arr, frame, frame.length());

		np::FloatArray2 forward(m_pConfig->nPixels, m_pConfig->nLines / 2);
		np::FloatArray2 backward(m_pConfig->nPixels, m_pConfig->nLines / 2);

		ippiCopy_32f_C1R(&frame(0, 0), sizeof(float) * 2 * m_pConfig->nPixels,
			forward, sizeof(float) * m_pConfig->nPixels,
			{ m_pConfig->nPixels, m_pConfig->nLines / 2 });
		ippiCopy_32f_C1R(&frame(0, 1), sizeof(float) * 2 * m_pConfig->nPixels,
			backward, sizeof(float) * m_pConfig->nPixels,
			{ m_pConfig->nPixels, m_pConfig->nLines / 2 });

		ippsDivC_32f_I(255.0f, forward, forward.length());
		ippsDivC_32f_I(255.0f, backward, backward.length());

		// Spline parameters	
		MKL_INT dorder = 1;
		MKL_INT nx = 100 + 1; // original data length
		MKL_INT nsite = 1000 + 1; // interpolated data length		
		float x[2] = { 0.0f, (float)nx - 1.0f }; // data range
		float* scoeff = new float[(nx - 1) * DF_PP_CUBIC];

		// Correlation buffers
		np::FloatArray corr_coefs0(nx);
		np::FloatArray corr_coefs(nsite);

		// Shifting along A-line dimension
		//int k = 1;
		for (int j = 0; j < nx; j++)
		{
			np::FloatArray fixed_vector, moving_vector;

			Ipp32f x_mean, x_std;
			Ipp32f y_mean, y_std;

			if (j < 0)
			{
				// Fixed
				fixed_vector = np::FloatArray((forward.size(0) + j) * forward.size(1));

				ippiCopy_32f_C1R(&forward(-j, 0), sizeof(float) * forward.size(0),
					fixed_vector, sizeof(float) * (forward.size(0) + j),
					{ forward.size(0) + j, forward.size(1) });
				ippsMeanStdDev_32f(fixed_vector, fixed_vector.length(), &x_mean, &x_std, ippAlgHintFast);
				ippsSubC_32f_I(x_mean, fixed_vector, fixed_vector.length());

				// Moving
				moving_vector = np::FloatArray((backward.size(0) + j) * backward.size(1));

				ippiCopy_32f_C1R(&backward(0, 0), sizeof(float) * backward.size(0),
					moving_vector, sizeof(float) * (backward.size(0) + j),
					{ backward.size(0) + j, backward.size(1) });
				ippsMeanStdDev_32f(moving_vector, moving_vector.length(), &y_mean, &y_std, ippAlgHintFast);
				ippsSubC_32f_I(y_mean, moving_vector, moving_vector.length());
			}
			else
			{
				// Fixed
				fixed_vector = np::FloatArray((forward.size(0) - j) * forward.size(1));

				ippiCopy_32f_C1R(&forward(0, 0), sizeof(float) * forward.size(0),
					fixed_vector, sizeof(float) * (forward.size(0) - j),
					{ forward.size(0) - j, forward.size(1) });
				ippsMeanStdDev_32f(fixed_vector, fixed_vector.length(), &x_mean, &x_std, ippAlgHintFast);
				ippsSubC_32f_I(x_mean, fixed_vector, fixed_vector.length());

				// Moving
				moving_vector = np::FloatArray((backward.size(0) - j) * backward.size(1));

				ippiCopy_32f_C1R(&backward(j, 0), sizeof(float) * backward.size(0),
					moving_vector, sizeof(float) * (backward.size(0) - j),
					{ backward.size(0) - j, backward.size(1) });
				ippsMeanStdDev_32f(moving_vector, moving_vector.length(), &y_mean, &y_std, ippAlgHintFast);
				ippsSubC_32f_I(y_mean, moving_vector, moving_vector.length());
			}

			// Correlation coefficient
			ippsMul_32f_I(fixed_vector, moving_vector, moving_vector.length());

			Ipp32f cov;
			ippsSum_32f(moving_vector, moving_vector.length(), &cov, ippAlgHintFast);
			cov = cov / (moving_vector.length() - 1);

			corr_coefs0(j) = cov / x_std / y_std * ((float)m_pConfig->nPixels / (float)(m_pConfig->nPixels - abs(j)));

			//k++;
		}

		// Spline interpolation		
		DFTaskPtr task1;
		dfsNewTask1D(&task1, nx, x, DF_UNIFORM_PARTITION, 1, corr_coefs0.raw_ptr(), DF_MATRIX_STORAGE_ROWS);
		dfsEditPPSpline1D(task1, DF_PP_CUBIC, DF_PP_NATURAL, DF_BC_NOT_A_KNOT, 0, DF_NO_IC, 0, scoeff, DF_NO_HINT);
		dfsConstruct1D(task1, DF_PP_SPLINE, DF_METHOD_STD);
		dfsInterpolate1D(task1, DF_INTERP, DF_METHOD_PP, nsite, x, DF_UNIFORM_PARTITION, 1, &dorder,
			DF_NO_APRIORI_INFO, corr_coefs.raw_ptr(), DF_MATRIX_STORAGE_ROWS, NULL);
		dfDeleteTask(&task1);
		mkl_free_buffers();

		delete[] scoeff;

		QFile file("corr.data");
		if (file.open(QIODevice::WriteOnly))
		{
			file.write(reinterpret_cast<char*>(corr_coefs.raw_ptr()), sizeof(float) * corr_coefs.length());
			file.close();
		}

		// Find correction index
		Ipp32f cmax; int cidx;
		//ippsMaxIndx_32f(&corr_coefs[1000], (corr_coefs.length() - 1) / 2 + 1, &cmax, &cidx);
		//float cidx1 = float(cidx) / 10.0f;

		ippsMaxIndx_32f(corr_coefs0, corr_coefs0.length(), &cmax, &cidx);
		float cidx1 = float(cidx); // / 10.0f;

		ippsMaxIndx_32f(corr_coefs, corr_coefs.length(), &cmax, &cidx);
		float cidx2 = float(cidx) / 10.0f;
		printf("%f\n", cidx2);

		m_pDoubleSpinBox_BiDirScanComp->setValue(cidx1);
	//});
	//auto_comp.detach();
#endif
}

void QStreamTab::changeCRSNonlinearityComp(bool toggled)
{
	m_pConfig->crsCompensation = toggled;
	if (toggled)
	{
		QFile file("crs_comp_idx.txt");
		if (false != file.open(QIODevice::ReadOnly))
		{
			QTextStream in(&file);

			int i = 0;
			while (!in.atEnd())
			{
				QString line = in.readLine();
				QStringList comp_idx = line.split('\t');

				m_pCRSCompIdx(i, 0) = comp_idx[0].toFloat();
				m_pCRSCompIdx(i, 1) = comp_idx[1].toFloat();
				i++;
			}

			file.close();
		}
		else
		{
			m_pCheckBox_CRSNonlinearityComp->setChecked(false);
			processMessage("No CRS nonlinearity compensation index file! (crs_comp_idx.txt)", true);
		}
	}
	else
	{
		for (int i = 0; i < m_pConfig->nPixels; i++)
		{
			m_pCRSCompIdx(i, 0) = i;
			m_pCRSCompIdx(i, 1) = 1;
		}
	}
}

void QStreamTab::changeCmosGain(int value)
{
	m_pConfig->cmosGain = value;
	m_pLabel_Gain->setText(QString("Gain: %1 dB  ").arg((double)value / 10.0, 2, 'f', 1));

	if (m_pOperationTab->getDataAcq()->getImagingSource())
		m_pOperationTab->getDataAcq()->getImagingSource()->setGain((double)value / 10.0);
}

void QStreamTab::changeCmosExposure(int value)
{
	m_pConfig->cmosExposure = value;
	m_pLabel_Exposure->setText(QString("Exposure: %1 msec  ").arg(value));	

	if (m_pOperationTab->getDataAcq()->getImagingSource())	
		m_pOperationTab->getDataAcq()->getImagingSource()->setExposure((double)value / 1000.0);
}


void QStreamTab::enableStitchingMode(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_StitchingMode->setText("Disable Stitching Mode");
			
		// Set enabled true for image stitching widgets
		//if (!m_pOperationTab->isAcquisitionButtonToggled())
		//{
			m_pLabel_XStep->setEnabled(true);
			m_pLineEdit_XStep->setEnabled(true);
			m_pLabel_YStep->setEnabled(true);
			m_pLineEdit_YStep->setEnabled(true);
		//}
		
		//m_pVisualizationTab->getImageView()->setHorizontalLine(1, m_pConfig->imageStichingMisSyncPos);
		//m_pVisualizationTab->visualizeImage();
	}
	else
	{
		// Set enabled false for image stitching widgets
		m_pLabel_XStep->setEnabled(false);
		m_pLineEdit_XStep->setEnabled(false);
		m_pLabel_YStep->setEnabled(false);
		m_pLineEdit_YStep->setEnabled(false);

		//m_pVisualizationTab->getImageView()->setHorizontalLine(0);
		//m_pVisualizationTab->visualizeImage();

		// Set text
		m_pCheckBox_StitchingMode->setText("Enable Stitching Mode");
	}
	
	QString str; str.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", m_nWrittenSamples, m_pConfig->imageSize, m_nAverageCount - 1, m_pConfig->imageAveragingFrames, m_nImageCount, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	m_pLabel_AcquisitionStatus->setText(str);

	//int accumulationCount = (m_nAcquiredFrames % m_pConfig->imageAccumulationFrames) + 1;
	//int averageCount = (m_nAcquiredFrames / m_pConfig->imageAccumulationFrames) + 1;

	//QString str1; str1.sprintf("Acc : %3d / %3d   Avg : %3d / %3d   Rec : %4d / %4d",
	//	accumulationCount, m_pConfig->imageAccumulationFrames, averageCount, m_pConfig->imageAveragingFrames,
	//	getOperationTab()->m_pMemoryBuffer->m_nRecordedImages,
	//	toggled ? m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep : 1);

	//m_pLabel_AcquisitionStatusMsg->setText(str1);

	//QString str; str.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", 0, m_pConfig->imageSize, 0, m_pConfig->imageAveragingFrames, 0, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	//m_pLabel_AcquisitionStatus->setText(str);
}

void QStreamTab::changeStitchingXStep(const QString &str)
{
	m_pConfig->imageStichingXStep = str.toInt();
	if (m_pConfig->imageStichingXStep < 1)
		m_pLineEdit_XStep->setText(QString::number(1));

	QString str1; str1.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", m_nWrittenSamples, m_pConfig->imageSize, m_nAverageCount - 1, m_pConfig->imageAveragingFrames, m_nImageCount, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	m_pLabel_AcquisitionStatus->setText(str1);
}

void QStreamTab::changeStitchingYStep(const QString &str)
{
	m_pConfig->imageStichingYStep = str.toInt();
	if (m_pConfig->imageStichingYStep < 1)
		m_pLineEdit_YStep->setText(QString::number(1));

	QString str1; str1.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", m_nWrittenSamples, m_pConfig->imageSize, m_nAverageCount - 1, m_pConfig->imageAveragingFrames, m_nImageCount, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
	m_pLabel_AcquisitionStatus->setText(str1);
}

void QStreamTab::processMessage(QString qmsg, bool is_error)
{
	m_pListWidget_MsgWnd->addItem(qmsg);
	m_pListWidget_MsgWnd->setCurrentRow(m_pListWidget_MsgWnd->count() - 1);

	if (is_error)
	{
		QMessageBox MsgBox(QMessageBox::Critical, "Error", qmsg);
		MsgBox.exec();
	}
}
