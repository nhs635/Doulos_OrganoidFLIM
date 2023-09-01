#ifndef PULSE_TRAIN_DIVIDER_H_
#define PULSE_TRAIN_DIVIDER_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class PulseTrainDivider
{
public:
	PulseTrainDivider();
    ~PulseTrainDivider();

	int slow;
	    
	const char* counterChannel;
	const char* sourceTerminal;

    bool initialize();
    void start();
    void stop();

public:
    callback2<const char*, bool> SendStatusMessage;

private:
    TaskHandle _taskHandle;
    void dumpError(int res, const char* pPreamble);
};

#endif // PULSE_TRAIN_DIVIDER_H_
