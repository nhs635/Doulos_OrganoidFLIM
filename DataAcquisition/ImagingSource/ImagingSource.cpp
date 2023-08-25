
#include "ImagingSource.h"

#define NUM_IMAGING_SOURCE_BUFFERS	20


ImagingSource::ImagingSource() :
	m_pGrabber(nullptr), m_pGainAbsolute(NULL), m_pExposureAbsolute(NULL), _dirty(true)
{
}

ImagingSource::~ImagingSource()
{
    if (_thread.joinable())
    {
        _running = false;
        _thread.join();
    }

	if (m_pGrabber)
	{
		delete m_pGrabber;
		m_pGrabber = nullptr;
	}
}

bool ImagingSource::initialize()
{
	// Initialize library
	if (!InitLibrary())
	{
		SendStatusMessage("Failed to initialize ImagingSource library.", true);
		return false;
	}

    return true;
}

bool ImagingSource::set_init()
{
	//if (_dirty)
	{
		if (!initialize())
		{			
			SendStatusMessage("Failed to initialize video capturing device.", true);
			return false;
		}

		SendStatusMessage("Video capturing device is successfully initialized.", false);
		_dirty = false;
	}

	return true;
}


bool ImagingSource::startAcquisition()
{
    if (_thread.joinable())
    {
        cout << "ERROR: Acquisition is already running" << endl;
        return false;
    }

    _thread = std::thread(&ImagingSource::run, this); // thread executing
    if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
    {
        cout << "ERROR: Failed to set acquisition thread priority: " << endl;
        return false;
    }

    SendStatusMessage("Video capturing thread is started.", false);

    return true;
}

void ImagingSource::stopAcquisition()
{
    if (_thread.joinable())
    {
        DidStopData();
        _thread.join();
    }

    SendStatusMessage("Video capturing thread is finished normally.", false);
}

void ImagingSource::setGain(double gain)
{
	if (m_pGrabber)
		if (m_pGainAbsolute != NULL)
			m_pGainAbsolute->setValue(gain);
}

void ImagingSource::setExposure(double exposure)
{
	if (m_pGrabber)
		if (m_pExposureAbsolute != NULL)
			m_pExposureAbsolute->setValue(exposure);
}


void ImagingSource::run()
{
	// Create imaging source grabber pointer
	if (!m_pGrabber)
		m_pGrabber = new Grabber;
	else
	{
		SendStatusMessage("Failed to create grabber object.", true);
		return;
	}

	// Get available video capture deivce
	Grabber::tVidCapDevListPtr pVidCapDevList = m_pGrabber->getAvailableVideoCaptureDevices();
	if (pVidCapDevList == 0 || pVidCapDevList->empty())
	{
		SendStatusMessage("Failed to find available imaging devices.", true); // m_pGrabber->getLastError().toString()
		return;
	}

	// Open available video capture device
	if (!m_pGrabber->openDev(pVidCapDevList->at(0))) // always open the first device
	{
		SendStatusMessage("Failed to open video capture device.", true);
		return;
	}

	// Get available video formats
	Grabber::tVidFmtListPtr pVidFmtList = m_pGrabber->getAvailableVideoFormats();
	if (pVidFmtList == 0) // No video formats available?
	{
		//std::cerr << "Error : " << m_pGrabber->getLastError().toString() << std::endl;
		SendStatusMessage("Failed to find available video format.", true);
		return;
	}
	else
	{
		// Set vidoe format as Y16(2048x2048): #21
		m_pGrabber->setVideoFormat(pVidFmtList->at(21));
	}

	// Create a FrameSnapSink with an image buffer format to eY16.
	tFrameSnapSinkPtr pSink = FrameSnapSink::create(eY16);

	// Set the sink.
	m_pGrabber->setSinkType(pSink);
	// Prepare the live mode, to get the output size of the sink.
	if (!m_pGrabber->prepareLive(false))
	{
		SendStatusMessage("Could not render the VideoFormat into a eY16sink.", true);
		return;
	}
	// Retrieve the output type and dimension of the sink
	// The dimension of the sink could be different from the VideoFormat, when
	// you use filters.
	FrameTypeInfo info;
	pSink->getOutputFrameType(info);

	BYTE* pBuf[NUM_IMAGING_SOURCE_BUFFERS] = {};
	tFrameQueueBufferList lst;
	for (int i = 0; i < NUM_IMAGING_SOURCE_BUFFERS; ++i)
	{
		// Create buffer
		pBuf[i] = new BYTE[info.buffersize];

		// Create a FrameQueueBuffer that objects that wraps the memory of our user buffer
		tFrameQueueBufferPtr ptr;
		Error err = createFrameQueueBuffer(ptr, info, pBuf[i], info.buffersize, NULL);
		if (err.isError()) 
		{
			SendStatusMessage("Failed to create buffer.", true);
			return;
		}
		lst.push_back(ptr);
	}

	// Get VCD property (gain, exposure)
	m_pGainAbsolute = m_pGrabber->getVCDPropertyInterface<IVCDAbsoluteValueProperty>(VCDID_Gain, VCDElement_Value);
	if (m_pGainAbsolute == NULL)
	{
		SendStatusMessage("Gain property has no absolute value interface.", true);
		return;
	}

	m_pExposureAbsolute = m_pGrabber->getVCDPropertyInterface<IVCDAbsoluteValueProperty>(VCDID_Exposure, VCDElement_Value);
	if (m_pExposureAbsolute == NULL)
	{
		SendStatusMessage("Exposure property has no absolute value interface.", true);
		return;
	}
		
	//<<run
	m_pGrabber->startLive(false);				// Start the grabber.
	
	// Acquire images
	unsigned int imageCnt = 0;
	unsigned int imageCntUpdate = 0;
	ULONG dwTickStart = 0, dwTickLastUpdate;
												
	_running = true;
	while (_running)
	{	
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			SetIllumPattern(imageCnt % 4);
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(max(33, (int)(m_pExposureAbsolute->getValue() * 1000.0))));
		Sleep(0);

		Error err = pSink->snapSingle(lst.at(imageCnt % NUM_IMAGING_SOURCE_BUFFERS));
		if (err.isError()) {
			std::cerr << "Failed to snap into buffers due to " << err.toString() << "\n";
			return;
		}

		// Copy frame data and send to callback function
		np::Uint16Array2 frame(info.dim.cx, info.dim.cy);
		memcpy(frame.raw_ptr(), (uint16_t*)lst.at(imageCnt % NUM_IMAGING_SOURCE_BUFFERS).get()->getPtr(), sizeof(uint16_t) * frame.length());
		DidAcquireData(imageCnt % 4, frame);

		// Acquisition Status
		if (!dwTickStart)
			dwTickStart = dwTickLastUpdate = GetTickCount();
		
		// Update counters
		imageCnt++;
		imageCntUpdate++;
		
		// Periodically update progress
		ULONG dwTickNow = GetTickCount();
		if (dwTickNow - dwTickLastUpdate > 5000)
		{
			double dRate, dRateUpdate;
		
			ULONG dwElapsed = dwTickNow - dwTickStart;
			ULONG dwElapsedUpdate = dwTickNow - dwTickLastUpdate;
			dwTickLastUpdate = dwTickNow;
		
			if (dwElapsed)
			{
				dRateUpdate = (double)imageCntUpdate / (double)dwElapsedUpdate * 1000.0;
		
				unsigned h = 0, m = 0, s = 0;
				if (dwElapsed >= 1000)
				{
					if ((s = dwElapsed / 1000) >= 60)	// Seconds
					{
						if ((m = s / 60) >= 60)			// Minutes
						{
							if (h = m / 60)				// Hours
								m %= 60;
						}
						s %= 60;
					}
				}
		
				char msg[256];
				sprintf(msg, "[Elapsed Time] %u:%02u:%02u [Acquired Frames] %d [Frame Rate] %.2f fps", h, m, s, imageCnt, dRateUpdate);
				SendStatusMessage(msg, false);
			}
		
			// reset
			imageCntUpdate = 0;
		}
	}

	// Stop live
	m_pGrabber->stopLive();					// Stop the grabber.

	// Delete grabber object
	if (m_pGrabber)
	{
		delete m_pGrabber;
		m_pGrabber = nullptr;

		m_pGainAbsolute = NULL;
		m_pExposureAbsolute = NULL;
	}

	ExitLibrary();
}
