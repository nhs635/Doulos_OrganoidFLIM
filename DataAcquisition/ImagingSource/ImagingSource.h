#ifndef _IMAGING_SOURCE_H_
#define _IMAGING_SOURCE_H_

#include "tisudshl.h"
#include "cmdhelper.h"
#include <iostream>
#include <thread>
#include <mutex>

#include <Common/array.h>
#include <Common/callback.h>

using namespace DShowLib;
using namespace std;
using namespace np;

class ImagingSource
{
// Constructor & destructor
public:
    explicit ImagingSource();
    virtual ~ImagingSource();

// Member functions
public:
    bool initialize();
	bool set_init();

    bool startAcquisition();
    void stopAcquisition();

	void setGain(double);
	void setExposure(double);
	double getGain() { return m_pGainAbsolute->getValue(); }
	double getExposure() { return m_pExposureAbsolute->getValue(); }
	bool isLive() { return m_pGrabber ? m_pGrabber->isLive() : false; }
	
private:
    void run();

// Member variables
public:
	callback<int> SetIllumPattern;
    callback2<int, const np::Uint16Array2 &> DidAcquireData;
    callback<void> DidStopData;
    callback2<const char*, bool> SendStatusMessage;

public:
    bool _running;
	std::mutex mutex_;

private:
	Grabber* m_pGrabber;
	
	// VCDProperty interface pointers
	tIVCDAbsoluteValuePropertyPtr m_pGainAbsolute;
	tIVCDAbsoluteValuePropertyPtr m_pExposureAbsolute;

    bool _dirty;
    std::thread _thread;
};

#endif
