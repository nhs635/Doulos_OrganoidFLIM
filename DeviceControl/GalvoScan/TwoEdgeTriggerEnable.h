#ifndef TWO_EDGE_TRIG_ENABLE_H_
#define TWO_EDGE_TRIG_ENABLE_H_

#include <Common/callback.h>


typedef void *TaskHandle;

class TwoEdgeTriggerEnable
{
public:
	TwoEdgeTriggerEnable();
    ~TwoEdgeTriggerEnable();

    const char* lines;

    bool initialize();
    void start();
    void stop();

public:
    callback2<const char*, bool> SendStatusMessage;
	
private:
    TaskHandle _taskHandle;
    void dumpError(int res, const char* pPreamble);
};

#endif // TWO_EDGE_TRIG_ENABLE_H_
