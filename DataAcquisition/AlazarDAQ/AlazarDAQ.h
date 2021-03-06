#ifndef ALAZAR_DAQ_H_
#define ALAZAR_DAQ_H_

#include <AlazarApi.h>
#include <AlazarError.h>
#include <AlazarCmd.h>

#include <Common/array.h>
#include <Common/callback.h>

#include <iostream>
#include <array>
#include <thread>

#define BUFFER_COUNT 4
#define MAX_MSG_LENGTH 2000


///typedef enum RETURN_CODE RETURN_CODE;
///typedef void * HANDLE;
///typedef void * dsp_module_handle;

class AlazarDAQ
{
// Methods
public:
    explicit AlazarDAQ();
    virtual ~AlazarDAQ();
	
public:
    bool initialize();
    bool set_init();
    void abort_acq();

    bool startAcquisition();
    void stopAcquisition();
	
public:
	inline bool is_initialized() const { return !_dirty; }

private:
	void run();

private:
	// Dump an error
	void dumpError(RETURN_CODE retCode, const char* pPreamble);
	void dumpErrorSystem(int res, const char* pPreamble);
		
// Variables
private:
	// Handle of Alazar board
	HANDLE boardHandle;

	// Array of buffer pointers
    std::array<uint16_t *, BUFFER_COUNT> BufferArray;
	
	// Initialization flag
	bool _dirty;
	
	// thread
	std::thread _thread;

public:
	unsigned long SystemId;
    int nChannels, nScans, nAlines;
	unsigned long VoltRange1, VoltRange2;
	unsigned long AcqRate;
	unsigned long TriggerDelay;
    unsigned long TriggerSlope;
	bool UseExternalClock;
    bool UseAutoTrigger;
	double frameRate;
	bool _running;
	
public:
	// callbacks	
	callback2<int, const void*> DidAcquireData;
	callback<void> DidStopData;
	callback2<const char*, bool> SendStatusMessage;
};

#endif // ALAZAR_DAQ_H_
