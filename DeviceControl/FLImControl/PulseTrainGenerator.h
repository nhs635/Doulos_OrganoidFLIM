#ifndef FLIM_TRIGGER_H_
#define FLIM_TRIGGER_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class FlimTrigger
{
public:
    FlimTrigger();
    ~FlimTrigger();

    int slow;
    const char* sourceTerminal;

    bool initialize();
    void start();
    void stop();

public:
    callback2<const char*, bool> SendStatusMessage;

private:
    const char* counterChannel;

    TaskHandle _taskHandle;
    void dumpError(int res, const char* pPreamble);
};

#endif // FLIM_TRIGGER_H_
