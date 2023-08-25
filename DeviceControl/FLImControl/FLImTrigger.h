#ifndef FLIM_TRIGGER_H_
#define FLIM_TRIGGER_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class FlimTrigger
{
public:
	FlimTrigger();
    ~FlimTrigger();

	double frequency;
    int finite_samps;

    const char* sourceTerminal;
	const char* counterChannel;
	const char* triggerSource;

    bool initialize();
    void start();
    void stop();

public:
    callback2<const char*, bool> SendStatusMessage;

private:
    TaskHandle _taskHandle;
    void dumpError(int res, const char* pPreamble);
};

#endif // FLIM_TRIGGER_H_
