
#include "QStreamTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Dialog/FlimCalibDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>

#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <DeviceControl/ZaberStage/ZaberStage.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>


QStreamTab::QStreamTab(QWidget *parent) :
	QDialog(parent), m_nWrittenSamples(0), m_nAcquiredFrames(0), m_nAverageCount(1),
	m_bIsStageTransition(false), m_nImageCount(0)
{
	// Set main window objects
	m_pMainWnd = dynamic_cast<MainWindow*>(parent);
	m_pConfig = m_pMainWnd->m_pConfiguration;

	// Create message window
	m_pListWidget_MsgWnd = new QListWidget(this);
	m_pListWidget_MsgWnd->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	m_pListWidget_MsgWnd->setStyleSheet("font: 7pt");
	m_pConfig->msgHandle += [&](const char* msg) {
		QString qmsg = QString::fromUtf8(msg);
		emit sendStatusMessage(qmsg, false);
	};
	connect(this, SIGNAL(sendStatusMessage(QString, bool)), this, SLOT(processMessage(QString, bool)));

	// Create image size widgets
	m_pLabel_YLines = new QLabel(this);
	m_pLabel_YLines->setText("#Y Lines  ");

	m_pRadioButton_136 = new QRadioButton(this);
	m_pRadioButton_136->setText("136");
	m_pRadioButton_270 = new QRadioButton(this);
	m_pRadioButton_270->setText("270");
	m_pRadioButton_540 = new QRadioButton(this);
	m_pRadioButton_540->setText("540");
	m_pRadioButton_1080 = new QRadioButton(this);
	m_pRadioButton_1080->setText("1080");

	m_pButtonGroup_YLines = new QButtonGroup(this);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_136, N_LINES_136);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_270, N_LINES_270);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_540, N_LINES_540);
	m_pButtonGroup_YLines->addButton(m_pRadioButton_1080, N_LINES_1080);

	switch (m_pConfig->nLines)
	{
	case N_LINES_136:
		m_pRadioButton_136->setChecked(true);
		m_pConfig->nLines = N_LINES_136;
		break;
	case N_LINES_270:
		m_pRadioButton_270->setChecked(true);
		m_pConfig->nLines = N_LINES_270;
		break;
	case N_LINES_540:
		m_pRadioButton_540->setChecked(true);
		m_pConfig->nLines = N_LINES_540;
		break;
	case N_LINES_1080:
		m_pRadioButton_1080->setChecked(true);
		m_pConfig->nLines = N_LINES_1080;
		break;
	default:
		m_pRadioButton_1080->setChecked(true);
		m_pConfig->nLines = N_LINES_1080;
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
#endif

	// CRS nonlinearity compensation
	m_pCheckBox_CRSNonlinearityComp = new QCheckBox(this);
	m_pCheckBox_CRSNonlinearityComp->setText("CRS Nonlinearity Compensation");

	//m_pCRSCompIdx = np::FloatArray2(m_pConfig->nPixels, 2);
	//for (int i = 0; i < m_pConfig->nPixels; i++)
	//{
	//	m_pCRSCompIdx(i, 0) = i;
	//	m_pCRSCompIdx(i, 1) = 1;
	//}

	//m_pCheckBox_CRSNonlinearityComp->setChecked(m_pConfig->crsCompensation);
	//if (m_pConfig->crsCompensation) changeCRSNonlinearityComp(true);

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


	QHBoxLayout *pHBoxLayout_YLines = new QHBoxLayout;
	pHBoxLayout_YLines->setSpacing(2);

	pHBoxLayout_YLines->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
	pHBoxLayout_YLines->addWidget(m_pLabel_YLines);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_136);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_270);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_540);
	pHBoxLayout_YLines->addWidget(m_pRadioButton_1080);

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
#endif

	QHBoxLayout *pHBoxLayout_CRS = new QHBoxLayout;
	pHBoxLayout_CRS->setSpacing(2);

	pHBoxLayout_CRS->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_CRS->addWidget(m_pCheckBox_CRSNonlinearityComp);

	QVBoxLayout *pVBoxLayout_ImageSize = new QVBoxLayout;
	pVBoxLayout_ImageSize->setSpacing(2);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_YLines);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_Averaging);
	pVBoxLayout_ImageSize->addWidget(m_pLabel_AcquisitionStatus);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_Bidir);
	pVBoxLayout_ImageSize->addItem(pHBoxLayout_CRS);

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

	// CRS persistant mode
	if (CRS_PERSISTANT_MODE)
	{
		getDeviceControlTab()->getResonantScanControl()->setChecked(true);
		getDeviceControlTab()->getResonantScanVoltageControl()->setChecked(true);
		getDeviceControlTab()->getResonantScanVoltageSpinBox()->setValue(m_pConfig->resonantScanVoltage);
	}


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

	m_pGroupBox_ImageSizeTab = new QGroupBox();
	m_pGroupBox_ImageSizeTab->setLayout(pVBoxLayout_ImageSize);
	m_pGroupBox_ImageSizeTab->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_ImageSizeTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pGroupBox_ImageSizeTab->setFixedWidth(332);

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
	m_syncFlimVisualization.allocate_queue_buffer(14, m_pConfig->nTimes, PROCESSING_BUFFER_SIZE); // FLIm Visualization

	// Set signal object
	setFlimAcquisitionCallback();
	setFlimProcessingCallback();
	setVisualizationCallback();


	// Create layout
	QGridLayout* pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(2);

	// Set layout
	pGridLayout->addWidget(m_pGroupBox_VisualizationTab, 0, 0, 6, 1);
	pGridLayout->addWidget(m_pGroupBox_OperationTab, 0, 1);
	pGridLayout->addWidget(m_pVisualizationTab->getVisualizationWidgetsBox(), 1, 1);
	pGridLayout->addWidget(m_pGroupBox_ImageSizeTab, 2, 1);
	pGridLayout->addWidget(m_pGroupBox_StitchingTab, 3, 1);
	pGridLayout->addWidget(m_pGroupBox_DeviceControlTab, 4, 1);
	pGridLayout->addWidget(m_pListWidget_MsgWnd, 5, 1);

	this->setLayout(pGridLayout);

	// Set timer for monitoring status
	m_pTimer_Monitoring = new QTimer(this);
	m_pTimer_Monitoring->start(100); // renew per 100 msec

	// Connect signal and slot
	connect(m_pButtonGroup_YLines, SIGNAL(buttonClicked(int)), this, SLOT(changeYLines(int)));
	connect(m_pLineEdit_Averaging, SIGNAL(textChanged(const QString &)), this, SLOT(changeAveragingFrame(const QString &)));
	connect(m_pDoubleSpinBox_BiDirScanComp, SIGNAL(valueChanged(double)), this, SLOT(changeBiDirScanComp(double)));
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
	m_nAverageCount = 1;

    m_pLabel_YLines->setEnabled(enabled);
	m_pRadioButton_136->setEnabled(enabled);
    m_pRadioButton_270->setEnabled(enabled);
    m_pRadioButton_540->setEnabled(enabled);
    m_pRadioButton_1080->setEnabled(enabled);
}

void QStreamTab::setAveragingWidgets(bool enabled)
{
	m_pLabel_Averaging->setEnabled(enabled);
	m_pLineEdit_Averaging->setEnabled(enabled);
	m_pCheckBox_CRSNonlinearityComp->setEnabled(enabled);

	if (getDeviceControlTab()->getZaberStageControl()->isChecked())
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
		m_nImageCount = 0;
	}
}

void QStreamTab::setFlimAcquisitionCallback()
{
	DataAcquisition* pDataAcq = m_pOperationTab->getDataAcq();
    pDataAcq->ConnectDaqAcquiredFlimData([&](int frame_count, const void* _frame_ptr) {
		
		if ((frame_count % (CRS_TOTAL_PIECES / CRS_DIR_FACTOR)) < CRS_VALID_PIECES)  // Pass only first third piece (CRS valid region)
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
				int frame_count0 = frame_count % (CRS_TOTAL_PIECES / CRS_DIR_FACTOR * (m_pConfig->nLines + GALVO_FLYING_BACK + 2));
				{
					if (m_pDeviceControlTab->getFlimCalibDlg())
					{
						int x, y;
						m_pVisualizationTab->getPixelPos(&x, &y);
						
						///printf("(%d %d) (%d %d) (%d %d)\n", frame_count, frame_count0, x, y, valid_buf, x0);

						int x0 = !(y % CRS_DIR_FACTOR) ? x : m_pConfig->nPixels - 1 - x;			
						if (frame_count0 % (CRS_TOTAL_PIECES / CRS_DIR_FACTOR) == x0 / N_TIMES)
							if (frame_count0 / (CRS_TOTAL_PIECES / CRS_DIR_FACTOR) == (y + GALVO_FLYING_BACK + 2))
								emit m_pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pulse_ptr, x0 % N_TIMES);
					}
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

                np::FloatArray2 intensity (flim_ptr + 0 * m_pConfig->nTimes, m_pConfig->nTimes, 5);
                np::FloatArray2 mean_delay(flim_ptr + 5 * m_pConfig->nTimes, m_pConfig->nTimes, 5);
                np::FloatArray2 lifetime  (flim_ptr + 10 * m_pConfig->nTimes, m_pConfig->nTimes, 4);
				
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

void QStreamTab::setVisualizationCallback()
{
    // Visualization Signal Objects ///////////////////////////////////////////////////////////////////////////////////////////
    m_pThreadVisualization->DidAcquireData += [&] (int frame_count) {

		MemoryBuffer *pMemBuff = m_pOperationTab->m_pMemoryBuffer;
		
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
					m_pTempIntensity = np::FloatArray2(m_pConfig->nPixels, 4 * effective_lines);
					memset(m_pTempIntensity, 0, sizeof(float) * m_pTempIntensity.length());
					m_pTempLifetime = np::FloatArray2(m_pConfig->nPixels, 4 * effective_lines);
					memset(m_pTempLifetime, 0, sizeof(float) * m_pTempLifetime.length());
					m_pNonNaNIndex = np::FloatArray2(m_pConfig->nPixels, 4 * effective_lines);
					ippsSet_32f(1.0f, m_pNonNaNIndex, m_pNonNaNIndex.length());
					//memset(m_pNonNaNIndex, 0, sizeof(float) * m_pNonNaNIndex.length());
				}

				// Data copy				
				np::FloatArray2 intensity(flim_data + 0 * m_pConfig->nTimes, m_pConfig->nTimes, 5);
				np::FloatArray2 lifetime(flim_data + 10 * m_pConfig->nTimes, m_pConfig->nTimes, 4);				
				tbb::parallel_for(tbb::blocked_range<size_t>(0, 4),
					[&](const tbb::blocked_range<size_t>& r) {
					for (size_t i = r.begin(); i != r.end(); ++i)
					{
						ippsAdd_32f_I(&intensity(0, i + 1), &m_pTempIntensity(0, i * effective_lines) + m_nWrittenSamples, m_pConfig->nTimes);
						ippsAdd_32f_I(&lifetime(0, i), &m_pTempLifetime(0, i * effective_lines) + m_nWrittenSamples, m_pConfig->nTimes);

						for (int j = 0; j < m_pConfig->nTimes; j++)
							if (intensity(j, i + 1) <= INTENSITY_THRES)
								*(&m_pNonNaNIndex(0, i * effective_lines) + m_nWrittenSamples + j) = 0.0f;
					}
				});			
				m_nWrittenSamples += m_pConfig->nTimes;

				// Update Status
				QString str; str.sprintf("Written: %7d / %7d   Avg: %3d / %3d   Rec: %3d / %3d", m_nWrittenSamples, m_pConfig->imageSize, m_nAverageCount - 1, m_pConfig->imageAveragingFrames, m_nImageCount, m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep);
				emit setAcquisitionStatus(str);
				
				if (m_nWrittenSamples == (m_pConfig->imageSize + (GALVO_FLYING_BACK + 2) * m_pConfig->nPixels))
				{
					// Averaging
					tbb::parallel_for(tbb::blocked_range<size_t>(0, 4),
						[&](const tbb::blocked_range<size_t>& r) {
						for (size_t i = r.begin(); i != r.end(); ++i)
						{
							ippsDivC_32f(&m_pTempIntensity(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_nAverageCount + 1, m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), m_pConfig->imageSize);
							ippsDivC_32f(&m_pTempLifetime(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_nAverageCount + 1, m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(), m_pConfig->imageSize);

							ippsMul_32f_I(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), m_pConfig->imageSize);
							ippsMul_32f_I(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(), m_pConfig->imageSize);

							//ippsDiv_32f(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), &m_pTempIntensity(0, i * effective_lines + GALVO_FLYING_BACK + 2),
							//	m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(), m_pConfig->imageSize);
							//ippsDiv_32f(&m_pNonNaNIndex(0, i * effective_lines + GALVO_FLYING_BACK + 2), &m_pTempLifetime(0, i * effective_lines + GALVO_FLYING_BACK + 2),
							//	m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(), m_pConfig->imageSize);
						}
					});
					m_nAverageCount++;

					// Bi-directional mirroring
					if (CRS_DIR_FACTOR == 2)
					{
						tbb::parallel_for(tbb::blocked_range<size_t>(0, 4),
							[&](const tbb::blocked_range<size_t>& r) {
							for (size_t i = r.begin(); i != r.end(); ++i)
							{
								ippiMirror_32f_C1IR(&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels, { m_pConfig->nPixels, m_pConfig->nLines / 2 }, ippAxsVertical);
								ippiMirror_32f_C1IR(&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels, { m_pConfig->nPixels, m_pConfig->nLines / 2 }, ippAxsVertical);
							}
						});

						// Bidirectional even & odd mismatch compensation
						tbb::parallel_for(tbb::blocked_range<size_t>(0, 4),
							[&](const tbb::blocked_range<size_t>& r) {
							for (size_t i = r.begin(); i != r.end(); ++i)
							{
								np::FloatArray2 left(m_pConfig->nPixels, m_pConfig->nLines / 2);
								np::FloatArray2 right(m_pConfig->nPixels, m_pConfig->nLines / 2);

								int shift = int(m_pConfig->biDirScanComp);
								float comp = m_pConfig->biDirScanComp - shift;
								
								if (m_pConfig->biDirScanComp >= 0)
								{
									// Intensity
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels, 
										&left(shift, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift, m_pConfig->nLines / 2 });
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&right(shift + 1, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift - 1, m_pConfig->nLines / 2 });

									ippsMulC_32f_I(1.0f - comp, left, left.length());
									ippsMulC_32f_I(comp, right, right.length());

									ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
										&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										{ m_pConfig->nPixels, m_pConfig->nLines / 2 });

									// Lifetime
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&left(shift, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift, m_pConfig->nLines / 2 });
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&right(shift + 1, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels - shift - 1, m_pConfig->nLines / 2 });

									ippsMulC_32f_I(1.0f - comp, left, left.length());
									ippsMulC_32f_I(comp, right, right.length());

									ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
										&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										{ m_pConfig->nPixels, m_pConfig->nLines / 2 });
								}
								else
								{
									// Intensity
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(-shift, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&left(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift, m_pConfig->nLines / 2 });
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisIntensity.at(i)(-shift + 1, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&right(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift - 1, m_pConfig->nLines / 2 });

									ippsMulC_32f_I(1.0f + comp, left, left.length());
									ippsMulC_32f_I(-comp, right, right.length());

									ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
										&m_pVisualizationTab->m_vecVisIntensity.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										{ m_pConfig->nPixels, m_pConfig->nLines / 2 });

									// Lifetime
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(-shift, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&left(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift, m_pConfig->nLines / 2 });
									ippiCopy_32f_C1R(&m_pVisualizationTab->m_vecVisLifetime.at(i)(-shift + 1, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										&right(0, 0), sizeof(float) * m_pConfig->nPixels, { m_pConfig->nPixels + shift - 1, m_pConfig->nLines / 2 });

									ippsMulC_32f_I(1.0f + comp, left, left.length());
									ippsMulC_32f_I(-comp, right, right.length());

									ippiAdd_32f_C1R(left, sizeof(float) * m_pConfig->nPixels, right, sizeof(float) * m_pConfig->nPixels,
										&m_pVisualizationTab->m_vecVisLifetime.at(i)(0, 1), sizeof(float) * CRS_DIR_FACTOR * m_pConfig->nPixels,
										{ m_pConfig->nPixels, m_pConfig->nLines / 2 });
								}
							}
						});
					}

					// Draw Images
					emit m_pVisualizationTab->drawImage();
					
					// Draw histogram statistics
					if (m_pDeviceControlTab->getFlimCalibDlg())
						emit m_pDeviceControlTab->getFlimCalibDlg()->plotHistogram(m_pVisualizationTab->m_vecVisIntensity.at(m_pConfig->flimEmissionChannel-1),
							m_pVisualizationTab->m_vecVisLifetime.at(m_pConfig->flimEmissionChannel-1));

					// Recording
					if (!m_bIsStageTransition)
					{
						if (m_nAverageCount > m_pConfig->imageAveragingFrames)
						{
							if (pMemBuff->m_bIsRecording)
							{
								int total_images = m_pCheckBox_StitchingMode->isChecked() ? m_pConfig->imageStichingXStep * m_pConfig->imageStichingYStep : 1;

								// Get buffer from writing queue
								float* image_ptr = pMemBuff->m_vectorWritingImageBuffer.at(pMemBuff->m_nRecordedFrame);

								if (image_ptr != nullptr)
								{
									// Body (Copying the frame data)
									for (int i = 0; i < 4; i++)
									{
										memcpy(image_ptr + i * m_pVisualizationTab->m_vecVisIntensity.at(i).length(), m_pVisualizationTab->m_vecVisIntensity.at(i).raw_ptr(),
											sizeof(float) * m_pVisualizationTab->m_vecVisIntensity.at(i).length());
										memcpy(image_ptr + (i + 4) * m_pVisualizationTab->m_vecVisLifetime.at(i).length(), m_pVisualizationTab->m_vecVisLifetime.at(i).raw_ptr(),
											sizeof(float) * m_pVisualizationTab->m_vecVisLifetime.at(i).length());
									}
									pMemBuff->increaseRecordedFrame();
								}

								// Stitiching - stage scanning
								if (++m_nImageCount < total_images)
								{
									m_bIsStageTransition = true;
									if (m_nImageCount % m_pConfig->imageStichingXStep != 0) // x move
									{
										double step = (double)((m_nImageCount / m_pConfig->imageStichingXStep + 1) % 2 ? +1 : -1) * (double)m_pConfig->zaberPullbackLength;
										emit getDeviceControlTab()->startStageScan(2, step);

									}
									else // y move
									{
										emit getDeviceControlTab()->startStageScan(1, m_pConfig->zaberPullbackLength);
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
										emit getDeviceControlTab()->startStageScan(2, 0);
										emit getDeviceControlTab()->startStageScan(1, 0);
									}
								}
							}

							m_nAverageCount = 1;
						}
					}
					else
					{
						m_nAverageCount = 1;
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

	if (getDeviceControlTab()->getZaberStage() && getDeviceControlTab()->getZaberStage()->getIsMoving())
	{
		m_pMainWnd->m_pStatusLabel_StageMoving->setText("Stage Moving O");
		m_pMainWnd->m_pStatusLabel_StageMoving->setStyleSheet("color: green;");
	}
	else
	{
		m_pMainWnd->m_pStatusLabel_StageMoving->setText("Stage Moving X");
		m_pMainWnd->m_pStatusLabel_StageMoving->setStyleSheet("color: red;");
	}
}


void QStreamTab::changeYLines(int n_lines)
{
	m_pConfig->nLines = n_lines;
	m_pConfig->imageSize = m_pConfig->nPixels * m_pConfig->nLines;
    m_pVisualizationTab->setObjects(m_pConfig->nLines);

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
