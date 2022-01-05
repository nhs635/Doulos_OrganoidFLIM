
#include "DataAcquisition.h"

#include <DataAcquisition/AlazarDAQ/AlazarDAQ.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>


DataAcquisition::DataAcquisition(Configuration* pConfig)
    : m_pDaq(nullptr), m_pFLIm(nullptr)
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
}

DataAcquisition::~DataAcquisition()
{
    if (m_pDaq) delete m_pDaq;
    if (m_pFLIm) delete m_pFLIm;
}


bool DataAcquisition::InitializeAcquistion()
{ 	 
    // Parameter settings for DAQ & Axsun Capture
	m_pDaq->SystemId = 1;
	m_pDaq->AcqRate = SAMPLE_RATE_1000MSPS;
	m_pDaq->nChannels = 1;
    m_pDaq->nScans = m_pConfig->nScans;
    m_pDaq->nAlines = m_pConfig->nTimes;
	m_pDaq->TriggerDelay = ALAZAR_TRIG_DELAY; // trigger delay
    m_pDaq->VoltRange1 = INPUT_RANGE_PM_400_MV;	

    // Initialization for DAQ
    if (!(m_pDaq->set_init()))
    {
        StopAcquisition();
        return false;
    }

    return true;
}

bool DataAcquisition::StartAcquisition()
{
	// delay

    // Start acquisition
    if (!(m_pDaq->startAcquisition()))
    {
        StopAcquisition();
        return false;
    }

    return true;
}

void DataAcquisition::StopAcquisition()
{
    // Stop thread
    m_pDaq->stopAcquisition();
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
