#ifndef GALVO_SCAN_H_
#define GALVO_SCAN_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class GalvoScan
{
public:
	GalvoScan();
	~GalvoScan();
	
    //int horizontal_size;

	double pp_voltage;
	double offset;
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
	//int nIter;

	double max_rate;
	double* data;
	
	TaskHandle _taskHandle;
	void dumpError(int res, const char* pPreamble);
};

#endif // GALVO_SCAN_H_
