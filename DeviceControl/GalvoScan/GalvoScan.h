#ifndef GALVO_SCAN_H_
#define GALVO_SCAN_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class GalvoScan
{
public:
	GalvoScan();
	~GalvoScan();
	
	double freq_fast;
	double pp_voltage_fast;
	double pp_voltage_slow;
	double offset_fast;
	double offset_slow;
	double max_rate;
	int step;
	
	bool initialize();
	void start();
	void stop();
		
	const char* physicalChannel;
	const char* sourceTerminal;
	const char* triggerSource;

	callback<void> startScan;
	callback<void> stopScan;

public:
    callback2<const char*, bool> SendStatusMessage;

private:	
	double* data;
	
	TaskHandle _taskHandle;
	void dumpError(int res, const char* pPreamble);
};

#endif // GALVO_SCAN_H_
