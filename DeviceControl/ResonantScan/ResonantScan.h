#ifndef RESONANT_SCAN_H_
#define RESONANT_SCAN_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class ResonantScan
{
public:
	ResonantScan();
	~ResonantScan();
	
	bool initialize();
	bool apply(double voltage);
	
public:
	callback2<const char*, bool> SendStatusMessage;

private:
	const char* physicalChannel;
	
	TaskHandle _taskHandle;
	void dumpError(int res, const char* pPreamble);
};

#endif // RESONANT_SCAN_H_