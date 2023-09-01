#ifndef PULSE_TRAIN_GENERATOR_H_
#define PULSE_TRAIN_GENERATOR_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class PulseTrainGenerator
{
public:
	PulseTrainGenerator();
    ~PulseTrainGenerator();

	double frequency;
    int finite_samps;
	    
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

#endif // PULSE_TRAIN_GENERATOR_H_
