
#include "FlimCalibDlg.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QOperationTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <iostream>
#include <thread>


static int roi_width = 0;

FlimCalibDlg::FlimCalibDlg(QWidget *parent) :
    QDialog(parent), m_pHistogramIntensity(nullptr), m_pHistogramLifetime(nullptr)
{
    // Set default size & frame
    setFixedSize(530, 720);
    setWindowFlags(Qt::Tool);
    setWindowTitle("FLIm Calibration");

    // Set main window objects
    m_pDeviceControlTab = dynamic_cast<QDeviceControlTab*>(parent);
    m_pConfig = m_pDeviceControlTab->getStreamTab()->getMainWnd()->m_pConfiguration;
    m_pFLIm = m_pDeviceControlTab->getStreamTab()->getOperationTab()->getDataAcq()->getFLIm();
		

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(10);

    // Create widgets for pulse view and FLIM calibration
    createPulseView();
    createCalibWidgets();
    createHistogram();

    // Set layout
    this->setLayout(m_pVBoxLayout);
}

FlimCalibDlg::~FlimCalibDlg()
{
    if (m_pHistogramIntensity) delete m_pHistogramIntensity;
    if (m_pHistogramLifetime) delete m_pHistogramLifetime;
}

void FlimCalibDlg::keyPressEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}


void FlimCalibDlg::createPulseView()
{
    // Create widgets for FLIM pulse view layout
    QGridLayout *pGridLayout_PulseView = new QGridLayout;
    pGridLayout_PulseView->setSpacing(2);

    // Create widgets for FLIM pulse view
	m_pImageView_PulseImage = new QImageView(ColorTable::colortable::parula, m_pConfig->nScans, m_pConfig->nTimes);
	m_pImageView_PulseImage->setFixedWidth(461);
	m_pImageView_PulseImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_pImageView_PulseImage->setHorizontalLine(1, 0);

	QLabel *pLabel_null = new QLabel(this);
	pLabel_null->setFixedWidth(37);

	m_pScope_PulseView = new QScope({ 0, (double)m_pConfig->nScans }, { 0, POWER_2(16) }, 2, 2, 1, 1, 0, 0, "", "");
    m_pScope_PulseView->setMinimumHeight(180);
    m_pScope_PulseView->getRender()->setGrid(8, 32, 1, true);
	m_pScope_PulseView->setDcLine(m_pFLIm->_params.bg);

    m_pCheckBox_ShowWindow = new QCheckBox(this);
    m_pCheckBox_ShowWindow->setText("Show Window");
    m_pCheckBox_ShowMeanDelay = new QCheckBox(this);
    m_pCheckBox_ShowMeanDelay->setText("Show Mean Delay");
	m_pCheckBox_ShowMeanDelay->setDisabled(true);
    m_pCheckBox_SplineView = new QCheckBox(this);
    m_pCheckBox_SplineView->setText("Spline View");
	m_pCheckBox_SplineView->setDisabled(true);
	m_pCheckBox_RoiSegmentView = new QCheckBox(this);
	m_pCheckBox_RoiSegmentView->setText("ROI Segment View");

    // Set layout
	QHBoxLayout *pHBoxLayout_ImageView = new QHBoxLayout;
	pHBoxLayout_ImageView->setSpacing(2);
	pHBoxLayout_ImageView->addWidget(pLabel_null);
	pHBoxLayout_ImageView->addWidget(m_pImageView_PulseImage);
	pHBoxLayout_ImageView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

	pGridLayout_PulseView->addItem(pHBoxLayout_ImageView, 0, 0, 1, 8);
    pGridLayout_PulseView->addWidget(m_pScope_PulseView, 1, 0, 1, 8);

    pGridLayout_PulseView->addWidget(m_pCheckBox_ShowWindow, 2, 0);
    pGridLayout_PulseView->addWidget(m_pCheckBox_ShowMeanDelay, 2, 1);
    pGridLayout_PulseView->addWidget(m_pCheckBox_SplineView, 2, 2);
    pGridLayout_PulseView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 3, 1, 4);
    pGridLayout_PulseView->addWidget(m_pCheckBox_RoiSegmentView, 2, 7);

    m_pVBoxLayout->addItem(pGridLayout_PulseView);

    // Connect
    connect(this, SIGNAL(plotRoiPulse(FLImProcess*, int)), this, SLOT(drawRoiPulse(FLImProcess*, int)));
	connect(this, SIGNAL(plotRoiPulse(float*, int)), this, SLOT(drawRoiPulse(float*, int)));
	connect(this, SIGNAL(plotHistogram(float*, float*)), this, SLOT(drawHistogram(float*, float*)));

    connect(m_pCheckBox_ShowWindow, SIGNAL(toggled(bool)), this, SLOT(showWindow(bool)));
    connect(m_pCheckBox_ShowMeanDelay, SIGNAL(toggled(bool)), this, SLOT(showMeanDelay(bool)));
    connect(m_pCheckBox_SplineView, SIGNAL(toggled(bool)), this, SLOT(splineView(bool)));
    connect(m_pCheckBox_RoiSegmentView, SIGNAL(toggled(bool)), this, SLOT(enableRoiSegmentView(bool)));
}

void FlimCalibDlg::createCalibWidgets()
{
    // Create widgets for FLIM calibration layout
    QGridLayout *pGridLayout_PulseView = new QGridLayout;
    pGridLayout_PulseView->setSpacing(2);

    // Create widgets for FLIM calibration
    m_pPushButton_CaptureBackground = new QPushButton(this);
    m_pPushButton_CaptureBackground->setText("Capture Background");

    m_pLineEdit_Background = new QLineEdit(this);
    m_pLineEdit_Background->setText(QString::number(m_pFLIm->_params.bg, 'f', 2));
    m_pLineEdit_Background->setFixedWidth(60);
    m_pLineEdit_Background->setAlignment(Qt::AlignCenter);
		
    m_pLabel_ChStart = new QLabel("Channel Start ", this);
    m_pLabel_DelayTimeOffset = new QLabel("Delay Time Offset", this);

    m_pLabel_Ch[0] = new QLabel("Ch 0 (IRF)", this);
    m_pLabel_Ch[0]->setFixedWidth(60);
    m_pLabel_Ch[0]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[1] = new QLabel("Ch 1 (390)", this);
    m_pLabel_Ch[1]->setFixedWidth(60);
    m_pLabel_Ch[1]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[2] = new QLabel("Ch 2 (450)", this);
    m_pLabel_Ch[2]->setFixedWidth(60);
    m_pLabel_Ch[2]->setAlignment(Qt::AlignCenter);
    m_pLabel_Ch[3] = new QLabel("Ch 3 (540)", this);
    m_pLabel_Ch[3]->setFixedWidth(60);
    m_pLabel_Ch[3]->setAlignment(Qt::AlignCenter);
	m_pLabel_Ch[4] = new QLabel("Ch 4 (618)", this);
	m_pLabel_Ch[4]->setFixedWidth(60);
	m_pLabel_Ch[4]->setAlignment(Qt::AlignCenter);

    for (int i = 0; i < 5; i++)
    {
        m_pSpinBox_ChStart[i] = new QMySpinBox(this);
        m_pSpinBox_ChStart[i]->setFixedWidth(60);
        m_pSpinBox_ChStart[i]->setRange(0.0, m_pConfig->nScans * m_pFLIm->_params.samp_intv);
        m_pSpinBox_ChStart[i]->setSingleStep(m_pFLIm->_params.samp_intv);
        m_pSpinBox_ChStart[i]->setValue((float)m_pFLIm->_params.ch_start_ind[i] * m_pFLIm->_params.samp_intv);
        m_pSpinBox_ChStart[i]->setDecimals(2);
        m_pSpinBox_ChStart[i]->setAlignment(Qt::AlignCenter);
						
        if (i != 0)
        {
            m_pLineEdit_DelayTimeOffset[i - 1] = new QLineEdit(this);
            m_pLineEdit_DelayTimeOffset[i - 1]->setFixedWidth(60);
            m_pLineEdit_DelayTimeOffset[i - 1]->setText(QString::number(m_pFLIm->_params.delay_offset[i - 1], 'f', 3));
            m_pLineEdit_DelayTimeOffset[i - 1]->setAlignment(Qt::AlignCenter);
        }
    }
    resetChStart0((double)m_pFLIm->_params.ch_start_ind[0] * (double)m_pFLIm->_params.samp_intv);
    resetChStart1((double)m_pFLIm->_params.ch_start_ind[1] * (double)m_pFLIm->_params.samp_intv);
    resetChStart2((double)m_pFLIm->_params.ch_start_ind[2] * (double)m_pFLIm->_params.samp_intv);
    resetChStart3((double)m_pFLIm->_params.ch_start_ind[3] * (double)m_pFLIm->_params.samp_intv);
	resetChStart4((double)m_pFLIm->_params.ch_start_ind[4] * (double)m_pFLIm->_params.samp_intv);
	
    m_pLabel_NanoSec[0] = new QLabel("nsec", this);
    m_pLabel_NanoSec[0]->setFixedWidth(25);
    m_pLabel_NanoSec[1] = new QLabel("nsec", this);
    m_pLabel_NanoSec[1]->setFixedWidth(25);

    // Set layout
    QHBoxLayout *pHBoxLayout_Background = new QHBoxLayout;
    pHBoxLayout_Background->setSpacing(2);
    pHBoxLayout_Background->addWidget(m_pPushButton_CaptureBackground);
    pHBoxLayout_Background->addWidget(m_pLineEdit_Background);
	
    pGridLayout_PulseView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0, 1, 4);
    pGridLayout_PulseView->addItem(pHBoxLayout_Background, 0, 4, 1, 3);
    pGridLayout_PulseView->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 7);

    pGridLayout_PulseView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_PulseView->addWidget(m_pLabel_ChStart, 2, 1);
    pGridLayout_PulseView->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 0);
    pGridLayout_PulseView->addWidget(m_pLabel_DelayTimeOffset, 3, 1, 1, 2);

    for (int i = 0; i < 5; i++)
    {
        pGridLayout_PulseView->addWidget(m_pLabel_Ch[i], 1, i + 2);
        pGridLayout_PulseView->addWidget(m_pSpinBox_ChStart[i], 2, i + 2);
    }
    for (int i = 0; i < 4; i++)
    {
        pGridLayout_PulseView->addWidget(m_pLineEdit_DelayTimeOffset[i], 3, i + 3);
    }
    pGridLayout_PulseView->addWidget(m_pLabel_NanoSec[0], 2, 7);
    pGridLayout_PulseView->addWidget(m_pLabel_NanoSec[1], 3, 7);

    m_pVBoxLayout->addItem(pGridLayout_PulseView);

    // Connect
    connect(m_pPushButton_CaptureBackground, SIGNAL(clicked(bool)), this, SLOT(captureBackground()));
    connect(m_pLineEdit_Background, SIGNAL(textChanged(const QString &)), this, SLOT(captureBackground(const QString &)));
    connect(m_pSpinBox_ChStart[0], SIGNAL(valueChanged(double)), this, SLOT(resetChStart0(double)));
    connect(m_pSpinBox_ChStart[1], SIGNAL(valueChanged(double)), this, SLOT(resetChStart1(double)));
    connect(m_pSpinBox_ChStart[2], SIGNAL(valueChanged(double)), this, SLOT(resetChStart2(double)));
    connect(m_pSpinBox_ChStart[3], SIGNAL(valueChanged(double)), this, SLOT(resetChStart3(double)));
	connect(m_pSpinBox_ChStart[4], SIGNAL(valueChanged(double)), this, SLOT(resetChStart4(double)));

    for (int i = 0; i < 4; i++)
        connect(m_pLineEdit_DelayTimeOffset[i], SIGNAL(textChanged(const QString &)), this, SLOT(resetDelayTimeOffset()));
}

void FlimCalibDlg::createHistogram()
{
    // Create widgets for histogram layout
    QHBoxLayout *pHBoxLayout_Histogram = new QHBoxLayout;
    pHBoxLayout_Histogram->setSpacing(2);

    uint8_t color[256];
    for (int i = 0; i < 256; i++)
        color[i] = i;
	
    // Create widgets for histogram (intensity)
    QGridLayout *pGridLayout_IntensityHistogram = new QGridLayout;
    pGridLayout_IntensityHistogram->setSpacing(1);

    m_pLabel_FluIntensity = new QLabel("Fluorescence Intensity (AU)", this);

    m_pRenderArea_FluIntensity = new QRenderArea(this);
    m_pRenderArea_FluIntensity->setSize({ 0, (double)N_BINS }, { 0, (double)m_pConfig->imageSize / 2 });
    m_pRenderArea_FluIntensity->setFixedSize(250, 150);
    m_pRenderArea_FluIntensity->setGrid(4, 16, 1);

    m_pHistogramIntensity = new Histogram(N_BINS, m_pConfig->imageSize);
	
    m_pColorbar_FluIntensity = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 256, 1, false);
    m_pColorbar_FluIntensity->drawImage(color);
    m_pColorbar_FluIntensity->getRender()->setFixedSize(250, 10);

    m_pLabel_FluIntensityMin = new QLabel(this);
    m_pLabel_FluIntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
    m_pLabel_FluIntensityMin->setAlignment(Qt::AlignLeft);
    m_pLabel_FluIntensityMax = new QLabel(this);
    m_pLabel_FluIntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    m_pLabel_FluIntensityMax->setAlignment(Qt::AlignRight);

    m_pLabel_FluIntensityMean = new QLabel(this);
    m_pLabel_FluIntensityMean->setFixedWidth(125);
    m_pLabel_FluIntensityMean->setText(QString("Mean: %1").arg(0.0, 4, 'f', 3));
    m_pLabel_FluIntensityMean->setAlignment(Qt::AlignCenter);
    m_pLabel_FluIntensityStd = new QLabel(this);
    m_pLabel_FluIntensityStd->setFixedWidth(125);
    m_pLabel_FluIntensityStd->setText(QString("Std: %1").arg(0.0, 4, 'f', 3));
    m_pLabel_FluIntensityStd->setAlignment(Qt::AlignCenter);

    // Create widgets for histogram (lifetime)
    QGridLayout *pGridLayout_LifetimeHistogram = new QGridLayout;
    pGridLayout_LifetimeHistogram->setSpacing(1);

    m_pLabel_FluLifetime = new QLabel("Fluorescence Lifetime (nsec)", this);

    m_pRenderArea_FluLifetime = new QRenderArea(this);
    m_pRenderArea_FluLifetime->setSize({ 0, (double)N_BINS }, { 0, (double)m_pConfig->imageSize / 2 });
    m_pRenderArea_FluLifetime->setFixedSize(250, 150);
    m_pRenderArea_FluLifetime->setGrid(4, 16, 1);

    m_pHistogramLifetime = new Histogram(N_BINS, m_pConfig->imageSize);

    m_pColorbar_FluLifetime = new QImageView(ColorTable::colortable(m_pConfig->flimLifetimeColorTable), 256, 1, false);
    m_pColorbar_FluLifetime->drawImage(color);
    m_pColorbar_FluLifetime->getRender()->setFixedSize(250, 10);

    m_pLabel_FluLifetimeMin = new QLabel(this);
    m_pLabel_FluLifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
    m_pLabel_FluLifetimeMin->setAlignment(Qt::AlignLeft);
    m_pLabel_FluLifetimeMax = new QLabel(this);
    m_pLabel_FluLifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    m_pLabel_FluLifetimeMax->setAlignment(Qt::AlignRight);

    m_pLabel_FluLifetimeMean = new QLabel(this);
    m_pLabel_FluLifetimeMean->setFixedWidth(125);
    m_pLabel_FluLifetimeMean->setText(QString("Mean: %1").arg(0.0, 4, 'f', 3));
    m_pLabel_FluLifetimeMean->setAlignment(Qt::AlignCenter);
    m_pLabel_FluLifetimeStd = new QLabel(this);
    m_pLabel_FluLifetimeStd->setFixedWidth(125);
    m_pLabel_FluLifetimeStd->setText(QString("Std: %1").arg(0.0, 4, 'f', 3));
    m_pLabel_FluLifetimeStd->setAlignment(Qt::AlignCenter);
	
    // Set layout
    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensity, 0, 0, 1, 4);
    pGridLayout_IntensityHistogram->addWidget(m_pRenderArea_FluIntensity, 1, 0, 1, 4);
    pGridLayout_IntensityHistogram->addWidget(m_pColorbar_FluIntensity->getRender(), 2, 0, 1, 4);

    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityMin, 3, 0);
    pGridLayout_IntensityHistogram->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 1, 1, 2);
    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityMax, 3, 3);

    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityMean, 4, 0, 1, 2);
    pGridLayout_IntensityHistogram->addWidget(m_pLabel_FluIntensityStd, 4, 2, 1, 2);


    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetime, 0, 0, 1, 4);
    pGridLayout_LifetimeHistogram->addWidget(m_pRenderArea_FluLifetime, 1, 0, 1, 4);
    pGridLayout_LifetimeHistogram->addWidget(m_pColorbar_FluLifetime->getRender(), 2, 0, 1, 4);

    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeMin, 3, 0);
    pGridLayout_LifetimeHistogram->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 1, 1, 2);
    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeMax, 3, 3);

    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeMean, 4, 0, 1, 2);
    pGridLayout_LifetimeHistogram->addWidget(m_pLabel_FluLifetimeStd, 4, 2, 1, 2);


    pHBoxLayout_Histogram->addItem(pGridLayout_IntensityHistogram);
    pHBoxLayout_Histogram->addItem(pGridLayout_LifetimeHistogram);

    m_pVBoxLayout->addItem(pHBoxLayout_Histogram);
}


void FlimCalibDlg::drawRoiPulse(FLImProcess* pFLIm, int aline)
{
	//// Data loading
	//np::FloatArray2 pulse_32f(pFLIm->_resize.crop_src.raw_ptr(), pFLIm->_resize.crop_src.size(0), pFLIm->_resize.crop_src.size(1));

	//// Reset size (if necessary)
	//if (roi_width != pulse_32f.size(0))
	//{
	//	m_pImageView_PulseImage->resetSize(pulse_32f.size(0), pulse_32f.size(1));
	//	m_pScope_PulseView->resetAxis({ 0, (double)pulse_32f.size(0) }, { 0, POWER_2(16) });		
	//	roi_width = pulse_32f.size(0);
	//}

	//// Data scaling (32f -> 8u)
	//int roi_width = ((pulse_32f.size(0) + 3) >> 2) << 2;

	//np::Uint8Array2 pulse_8u(roi_width, pulse_32f.size(1));
	//ippiScale_32f8u_C1R(pulse_32f.raw_ptr(), sizeof(float) * pulse_32f.size(0),
	//	pulse_8u.raw_ptr(), sizeof(uint8_t) * pulse_8u.size(0),
	//	{ pulse_32f.size(0), pulse_32f.size(1) }, 0.0f, 65535.0f);

	//// Window
	//if (m_pCheckBox_ShowWindow->isChecked())
	//{
	//	int offset = m_pCheckBox_RoiSegmentView->isChecked() ? m_pFLIm->_params.ch_start_ind[0] : 0;
	//	for (int i = 0; i < 6; i++)
	//	{
	//		m_pScope_PulseView->getRender()->m_pWinLineInd[i] = m_pFLIm->_params.ch_start_ind[i] - offset;
	//		m_pImageView_PulseImage->getRender()->m_pVLineInd[i] = m_pFLIm->_params.ch_start_ind[i] - offset;
	//	}
	//}

	//// Mean delay
	//if (m_pCheckBox_ShowMeanDelay->isChecked())
	//{
	//	int offset = m_pCheckBox_RoiSegmentView->isChecked() ? pFLIm->_params.ch_start_ind[0] : 0;
	//	for (int i = 0; i < 5; i++)
	//		m_pScope_PulseView->getRender()->m_pMdLineInd[i] = pFLIm->_lifetime.mean_delay(aline, i) - offset;
	//}

	//// ROI image
	//m_pImageView_PulseImage->drawImage(pulse_8u.raw_ptr());

	//// ROI pulse
	//m_pScope_PulseView->drawData(&pulse_32f(0, aline));
}

void FlimCalibDlg::drawRoiPulse(float* pulse_ptr, int aline)
{
	// Data loading
	np::FloatArray2 pulse_32f;
	if (!m_pCheckBox_RoiSegmentView->isChecked())
	{
		pulse_32f = np::FloatArray2(m_pConfig->nScans, m_pConfig->nTimes);
		memcpy(pulse_32f, pulse_ptr, sizeof(float) * m_pConfig->bufferSize);
	}
	else
	{
		np::FloatArray2 pulse_temp = np::FloatArray2(m_pConfig->nScans, m_pConfig->nTimes);
		memcpy(pulse_temp, pulse_ptr, sizeof(float) * m_pConfig->bufferSize);
		int _roi_width = m_pFLIm->_params.ch_start_ind[5] - m_pFLIm->_params.ch_start_ind[0];
		
		pulse_32f = np::FloatArray2(pulse_ptr, _roi_width, m_pConfig->nTimes);
		ippiCopy_32f_C1R(&pulse_temp(m_pFLIm->_params.ch_start_ind[0], 0), sizeof(float) * m_pConfig->nScans,
			&pulse_32f(0, 0), sizeof(float) * _roi_width, { _roi_width, m_pConfig->nTimes });
	}

	// Reset size (if necessary)	
	if (roi_width != pulse_32f.size(0))
	{
		m_pImageView_PulseImage->resetSize(pulse_32f.size(0), pulse_32f.size(1));
		m_pScope_PulseView->resetAxis({ 0, (double)pulse_32f.size(0) }, { 0, POWER_2(16) });
		roi_width = pulse_32f.size(0);
	}

	// Data scaling (32f -> 8u)
	np::Uint8Array2 pulse_8u(((pulse_32f.size(0) + 3) >> 2) << 2, pulse_32f.size(1));
	ippiScale_32f8u_C1R(pulse_32f.raw_ptr(), sizeof(float) * pulse_32f.size(0),
		pulse_8u.raw_ptr(), sizeof(uint8_t) * pulse_8u.size(0),
		{ pulse_32f.size(0), pulse_32f.size(1) }, 0.0f, 65535.0f);

	// Window
	if (m_pCheckBox_ShowWindow->isChecked())
	{
		int offset = m_pCheckBox_RoiSegmentView->isChecked() ? m_pFLIm->_params.ch_start_ind[0] : 0;
		for (int i = 0; i < 6; i++)
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[i] = m_pFLIm->_params.ch_start_ind[i] - offset;
			m_pImageView_PulseImage->getRender()->m_pVLineInd[i] = m_pFLIm->_params.ch_start_ind[i] - offset;
		}
	}

	// Mean delay
	if (m_pCheckBox_ShowMeanDelay->isChecked())
	{
		// Calculate
		np::FloatArray mean_delay(5);
		for (int i = 0; i < 5; i++)
		{
			int offset, width, roi_width, left, max_idx;
			float md_temp;
			offset = m_pFLIm->_params.ch_start_ind[i] - m_pFLIm->_params.ch_start_ind[0];

			const float* pulse = &pulse_32f(offset, aline);
			m_pFLIm->_lifetime.WidthIndex_32f(pulse, 0.5f, m_pFLIm->_resize.pulse_roi_length, max_idx, width);
			roi_width = (int)round(m_pFLIm->_params.width_factor * width);
			left = (int)floor(roi_width / 2);
			
			m_pFLIm->_lifetime.MeanDelay_32f(m_pFLIm->_resize, pulse, offset, aline, max_idx, roi_width, left, md_temp);
			mean_delay(i) = md_temp + (float)m_pFLIm->_params.ch_start_ind[i];
		}

		// Visualize
		{
			int offset = m_pCheckBox_RoiSegmentView->isChecked() ? m_pFLIm->_params.ch_start_ind[0] : 0;
			for (int i = 0; i < 5; i++)
				m_pScope_PulseView->getRender()->m_pMdLineInd[i] = mean_delay(i) - offset;
		}
	}

	// ROI image
	m_pImageView_PulseImage->drawImage(pulse_8u.raw_ptr());
	m_pImageView_PulseImage->setHorizontalLine(1, aline);

	// ROI pulse
	m_pScope_PulseView->drawData(&pulse_32f(0, aline));
}

void FlimCalibDlg::drawHistogram(float* pIntensity, float* pLifetime)
{
	// Histogram
	float* scanIntensity = pIntensity;
	float* scanLifetime = pLifetime;

	(*m_pHistogramIntensity)(scanIntensity, m_pRenderArea_FluIntensity->m_pData,
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min,
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);
	(*m_pHistogramLifetime)(scanLifetime, m_pRenderArea_FluLifetime->m_pData,
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min,
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);
	m_pRenderArea_FluIntensity->update();
	m_pRenderArea_FluLifetime->update();

	m_pLabel_FluIntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
	m_pLabel_FluIntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
	m_pLabel_FluLifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
	m_pLabel_FluLifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));

	m_pColorbar_FluLifetime->resetColormap(ColorTable::colortable(m_pConfig->flimLifetimeColorTable));

	Ipp32f mean, stdev;
	FloatArray scanIntensity0(m_pConfig->nTimes); memset(scanIntensity0, 0, sizeof(float) * m_pConfig->nTimes);
	FloatArray scanLifetime0(m_pConfig->nTimes); memset(scanLifetime0, 0, sizeof(float) * m_pConfig->nTimes);
	int it_n = 0, lt_n = 0;
	for (int i = 0; i < m_pConfig->nTimes; i++)
	{
		if (scanIntensity[i] != 0.0f) //(!std::isnan(scanIntensity[i]))
			scanIntensity0[it_n++] = scanIntensity[i];
		if (scanLifetime[i] != 0.0f) // (!std::isnan(scanLifetime[i]))
			scanLifetime0[lt_n++] = scanLifetime[i];
	}

	ippsMeanStdDev_32f(scanIntensity0, it_n, &mean, &stdev, ippAlgHintFast);
	mean = (mean > 1000.0f) ? 0.000f : mean; stdev = (stdev > 1000.0f) ? 0.000f : stdev;
	m_pLabel_FluIntensityMean->setText(QString("Mean: %1").arg(mean, 4, 'f', 3));
	m_pLabel_FluIntensityStd->setText(QString("Std: %1").arg(stdev, 4, 'f', 3));
	ippsMeanStdDev_32f(scanLifetime0, lt_n, &mean, &stdev, ippAlgHintFast);
	mean = (mean > 1000.0f) ? 0.000f : mean; stdev = (stdev > 1000.0f) ? 0.000f : stdev;
	m_pLabel_FluLifetimeMean->setText(QString("Mean: %1").arg(mean, 4, 'f', 3));
	m_pLabel_FluLifetimeStd->setText(QString("Std: %1").arg(stdev, 4, 'f', 3));
}

void FlimCalibDlg::showWindow(bool checked)
{
    if (checked)
    {
		int offset = m_pCheckBox_RoiSegmentView->isChecked() ? m_pFLIm->_params.ch_start_ind[0] : 0;

		m_pScope_PulseView->setWindowLine(6);
		m_pImageView_PulseImage->setVerticalLine(6);
		for (int i = 0; i < 6; i++)
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[i] = m_pFLIm->_params.ch_start_ind[i] - offset;
			m_pImageView_PulseImage->getRender()->m_pVLineInd[i] = m_pFLIm->_params.ch_start_ind[i] - offset;
		}
    }
    else
    {
        m_pScope_PulseView->setWindowLine(0);
		m_pImageView_PulseImage->setVerticalLine(0);
    }
    m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::showMeanDelay(bool checked)
{
    if (checked)
    {
        m_pScope_PulseView->setMeanDelayLine(5);
		for (int i = 0; i < 5; i++)
			m_pScope_PulseView->getRender()->m_pMdLineInd[i] = m_pFLIm->_lifetime.mean_delay(0, i) - m_pFLIm->_params.ch_start_ind[0];
    }
    else
    {
        m_pScope_PulseView->setMeanDelayLine(0);
    }
    m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::splineView(bool checked)
{
    //if (m_pCheckBox_ShowWindow->isChecked())
    //{
    //    int* ch_ind = (!checked) ? m_pFLIm->_params.ch_start_ind : m_pFLIm->_resize.ch_start_ind1;

    //    m_pScope_PulseView->setWindowLine(5,
    //        0, ch_ind[1] - ch_ind[0], ch_ind[2] - ch_ind[0],
    //           ch_ind[3] - ch_ind[0], ch_ind[4] - ch_ind[0]);
    //}
    //else
    //{
    //    m_pScope_PulseView->setWindowLine(0);
    //}
    //drawRoiPulse(m_pFLIm, 0);
	
    //m_pCheckBox_ShowMask->setDisabled(checked);
}

void FlimCalibDlg::enableRoiSegmentView(bool enabled)
{
	if (!enabled)
		m_pCheckBox_ShowMeanDelay->setChecked(false);
	m_pCheckBox_ShowMeanDelay->setEnabled(enabled);
	showWindow(m_pCheckBox_ShowWindow->isChecked());
}


void FlimCalibDlg::captureBackground()
{
    Ipp32f bg;
    ippsMean_32f(m_pScope_PulseView->getRender()->m_pData, m_pScope_PulseView->getRender()->m_sizeGraph.width(), &bg, ippAlgHintFast);

    m_pLineEdit_Background->setText(QString::number(bg, 'f', 2));
}

void FlimCalibDlg::captureBackground(const QString &str)
{
    float bg = str.toFloat();
	
	m_pScope_PulseView->setDcLine(bg);
    m_pFLIm->_params.bg = bg;
    m_pConfig->flimBg = bg;
}

void FlimCalibDlg::resetChStart0(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);
	
    m_pFLIm->_params.ch_start_ind[0] = ch_ind;
    m_pConfig->flimChStartInd[0] = ch_ind;
	
    ///printf("[Ch 0] %d %d %d %d\n",
    ///    m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
    ///    m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

    m_pSpinBox_ChStart[1]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);
			
    if (m_pCheckBox_ShowWindow->isChecked())
    {
        ///float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;

		if (!m_pCheckBox_RoiSegmentView->isChecked())
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[0] = ch_ind; // (int)round((float)(ch_ind));
			m_pImageView_PulseImage->getRender()->m_pVLineInd[0] = ch_ind;
		}
		else
		{
			for (int i = 0; i < 5; i++)
			{
				m_pScope_PulseView->getRender()->m_pWinLineInd[0] = m_pFLIm->_params.ch_start_ind[i] - ch_ind;
				m_pImageView_PulseImage->getRender()->m_pVLineInd[0] = m_pFLIm->_params.ch_start_ind[i] - ch_ind;
			}
			m_pScope_PulseView->getRender()->m_pWinLineInd[5] = m_pFLIm->_params.ch_start_ind[4] - ch_ind + FLIM_CH_START_6;
			m_pImageView_PulseImage->getRender()->m_pVLineInd[5] = m_pFLIm->_params.ch_start_ind[4] - ch_ind + FLIM_CH_START_6;
		}
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::resetChStart1(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);
	
    m_pFLIm->_params.ch_start_ind[1] = ch_ind;
    m_pConfig->flimChStartInd[1] = ch_ind;

    m_pFLIm->_resize.initiated = false;

    ///printf("[Ch 1] %d %d %d %d\n",
    ///    m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
    ///    m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

    m_pSpinBox_ChStart[0]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);
    m_pSpinBox_ChStart[2]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
        ///float factor = (!m_pCheckBox_SplineView->isChecked()) ? 1 : m_pFLIm->_resize.ActualFactor;
		if (!m_pCheckBox_RoiSegmentView->isChecked())
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[1] = ch_ind; /// (int)round((float)(ch_ind)* factor);
			m_pImageView_PulseImage->getRender()->m_pVLineInd[1] = ch_ind;
		}
		else
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[1] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
			m_pImageView_PulseImage->getRender()->m_pVLineInd[1] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
		}
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::resetChStart2(double start)
{
    int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);

    m_pFLIm->_params.ch_start_ind[2] = ch_ind;
    m_pConfig->flimChStartInd[2] = ch_ind;

    m_pFLIm->_resize.initiated = false;

    ///printf("[Ch 2] %d %d %d %d\n",
    ///    m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
    ///    m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

    m_pSpinBox_ChStart[1]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);
    m_pSpinBox_ChStart[3]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);

    if (m_pCheckBox_ShowWindow->isChecked())
    {
		if (!m_pCheckBox_RoiSegmentView->isChecked())
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[2] = ch_ind; /// (int)round((float)(ch_ind)* factor);
			m_pImageView_PulseImage->getRender()->m_pVLineInd[2] = ch_ind;
		}
		else
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[2] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
			m_pImageView_PulseImage->getRender()->m_pVLineInd[2] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
		}
    }

    m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::resetChStart3(double start)
{
	int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);

	m_pFLIm->_params.ch_start_ind[3] = ch_ind;
	m_pConfig->flimChStartInd[3] = ch_ind;

	m_pFLIm->_resize.initiated = false;

	///printf("[Ch 3] %d %d %d %d\n",
	///    m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
	///    m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

	m_pSpinBox_ChStart[2]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);
	m_pSpinBox_ChStart[4]->setMinimum((double)(ch_ind + 10) * (double)m_pFLIm->_params.samp_intv);

	if (m_pCheckBox_ShowWindow->isChecked())
	{
		if (!m_pCheckBox_RoiSegmentView->isChecked())
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[3] = ch_ind; /// (int)round((float)(ch_ind)* factor);
			m_pImageView_PulseImage->getRender()->m_pVLineInd[3] = ch_ind;
		}
		else
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[3] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
			m_pImageView_PulseImage->getRender()->m_pVLineInd[3] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
		}
	}

	m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::resetChStart4(double start)
{
	int ch_ind = (int)round(start / m_pFLIm->_params.samp_intv);

	m_pFLIm->_params.ch_start_ind[4] = ch_ind;
	m_pFLIm->_params.ch_start_ind[5] = ch_ind + FLIM_CH_START_6;
	m_pConfig->flimChStartInd[4] = ch_ind;

	///printf("[Ch 4] %d %d %d %d\n",
	///    m_pFLIm->_params.ch_start_ind[0], m_pFLIm->_params.ch_start_ind[1],
	///    m_pFLIm->_params.ch_start_ind[2], m_pFLIm->_params.ch_start_ind[3]);

	m_pSpinBox_ChStart[3]->setMaximum((double)(ch_ind - 10) * (double)m_pFLIm->_params.samp_intv);

	if (m_pCheckBox_ShowWindow->isChecked())
	{
		if (!m_pCheckBox_RoiSegmentView->isChecked())
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[4] = ch_ind; /// (int)round((float)(ch_ind)* factor);
			m_pImageView_PulseImage->getRender()->m_pVLineInd[4] = ch_ind;
			m_pScope_PulseView->getRender()->m_pWinLineInd[5] = ch_ind + FLIM_CH_START_6;
			m_pImageView_PulseImage->getRender()->m_pVLineInd[5] = ch_ind + FLIM_CH_START_6;
		}
		else
		{
			m_pScope_PulseView->getRender()->m_pWinLineInd[4] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
			m_pImageView_PulseImage->getRender()->m_pVLineInd[4] = ch_ind - m_pFLIm->_params.ch_start_ind[0];
			m_pScope_PulseView->getRender()->m_pWinLineInd[5] = ch_ind - m_pFLIm->_params.ch_start_ind[0] + FLIM_CH_START_6;
			m_pImageView_PulseImage->getRender()->m_pVLineInd[5] = ch_ind - m_pFLIm->_params.ch_start_ind[0] + FLIM_CH_START_6;
		}
	}

	m_pScope_PulseView->getRender()->update();
}

void FlimCalibDlg::resetDelayTimeOffset()
{
    float delay_offset[4];
    for (int i = 0; i < 4; i++)
    {
        delay_offset[i] = m_pLineEdit_DelayTimeOffset[i]->text().toFloat();

        m_pFLIm->_params.delay_offset[i] = delay_offset[i];
        m_pConfig->flimDelayOffset[i] = delay_offset[i];
    }
}
