
#include "DataAcquisition.h"

#include <DataAcquisition/AlazarDAQ/AlazarDAQ.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/QpiProcess/QpiProcess.h>
#include <DataAcquisition/ImagingSource/ImagingSource.h>

#include <QImage>


DataAcquisition::DataAcquisition(Configuration* pConfig)
    : m_pDaq(nullptr), m_pFLIm(nullptr), m_pQpi(nullptr) , m_pImagingSource(nullptr)
{
    m_pConfig = pConfig;

    // Create SignatecDAQ object
    m_pDaq = new AlazarDAQ;
    m_pDaq->DidStopData += [&]() { m_pDaq->_running = false; };

    // Create FLIm process object
    m_pFLIm = new FLImProcess;
	m_pFLIm->SendStatusMessage += [&](const char* msg) { m_pConfig->msgHandle(msg); };
	m_pFLIm->_resize.SendStatusMessage += [&](const char* msg) { m_pConfig->msgHandle(msg); };
    m_pFLIm->setParameters(m_pConfig);
    m_pFLIm->_resize(np::FloatArray2(m_pConfig->nScans, m_pConfig->nTimes), m_pFLIm->_params);
    m_pFLIm->loadMaskData();

	// Create QPI process object
	m_pQpi = new QpiProcess(CMOS_WIDTH, CMOS_HEIGHT, PUPIL_RADIUS, m_pConfig->regL2amp, m_pConfig->regL2phase, m_pConfig->regTv);
	
	// Create Brightfield camera object
	m_pImagingSource = new ImagingSource;
	m_pImagingSource->DidStopData += [&]() { m_pImagingSource->_running = false; };
}

DataAcquisition::~DataAcquisition()
{
    if (m_pDaq) delete m_pDaq;
    if (m_pFLIm) delete m_pFLIm;
	if (m_pQpi) delete m_pQpi;
	if (m_pImagingSource) delete m_pImagingSource;
}


bool DataAcquisition::InitializeAcquistion(bool is_flim)
{ 	 
	if (is_flim)
	{
		// Parameter settings for DAQ & Axsun Capture
		m_pDaq->SystemId = 1;
		m_pDaq->AcqRate = SAMPLE_RATE_500MSPS;
		m_pDaq->nChannels = 1;
		m_pDaq->nScans = m_pConfig->nScans;
		m_pDaq->nAlines = m_pConfig->nTimes;
		m_pDaq->TriggerDelay = 0; // trigger delay
		m_pDaq->VoltRange1 = INPUT_RANGE_PM_400_MV; // INPUT_RANGE_PM_400_MV INPUT_RANGE_PM_1_V INPUT_RANGE_PM_2_V

		// Initialization for DAQ
		if (!(m_pDaq->set_init()))
		{
			StopAcquisition();
			return false;
		}
	}
	else
	{
		// Initialization for Brightfield
		if (!(m_pImagingSource->set_init()))
		{
			StopAcquisition();
			return false;
		}
	}

    return true;
}

bool DataAcquisition::StartAcquisition(bool is_flim)
{
	// delay

	if (is_flim)
	{
		// Start acquisition
		if (!(m_pDaq->startAcquisition()))
		{
			StopAcquisition();
			return false;
		}
	}
	else
	{
		// Start acquisition
		if (!(m_pImagingSource->startAcquisition()))
		{
			StopAcquisition();
			return false;
		}
	}

    return true;
}

void DataAcquisition::StopAcquisition(bool is_flim)
{
    // Stop thread
	if (is_flim)
		m_pDaq->stopAcquisition();
	else
		m_pImagingSource->stopAcquisition();
}


void DataAcquisition::ConnectDaqAcquiredFlimData(const std::function<void(int, const void*)> &slot)
{
    m_pDaq->DidAcquireData += slot;
}

void DataAcquisition::ConnectDaqStopFlimData(const std::function<void(void)> &slot)
{
    m_pDaq->DidStopData += slot;
}

void DataAcquisition::ConnectDaqSendStatusMessage(const std::function<void(const char*, bool)> &slot)
{
    m_pDaq->SendStatusMessage += slot;
}

void DataAcquisition::ConnectBrightfieldAcquiredFlimData(const std::function<void(int, const np::Uint16Array2 &)> &slot)
{
	m_pImagingSource->DidAcquireData += slot;
}

void DataAcquisition::ConnectBrightfieldStopFlimData(const std::function<void(void)> &slot)
{
	m_pImagingSource->DidStopData += slot;
}

void DataAcquisition::ConnectBrightfieldSendStatusMessage(const std::function<void(const char*, bool)> &slot)
{
	m_pImagingSource->SendStatusMessage += slot;
}
