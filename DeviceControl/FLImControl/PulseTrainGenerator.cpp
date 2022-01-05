
#include "FlimTrigger.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


FlimTrigger::FlimTrigger() :
    _taskHandle(nullptr),
    slow(4),
    counterChannel(""), //NI_FLIM_TRIG_CHANNEL
    sourceTerminal("100MHzTimeBase")
{
}


FlimTrigger::~FlimTrigger()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool FlimTrigger::initialize()
{	
    SendStatusMessage("Initializing NI Counter for triggering FLIm laser...", false);

    int lowTicks = (int)(ceil(slow / 2));
    int highTicks = (int)(floor(slow / 2));
    uint64_t sampsPerChan = 120000;
    int res;

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    if ((res = DAQmxCreateCOPulseChanTicks(_taskHandle, counterChannel, NULL, sourceTerminal, DAQmx_Val_Low, 0, lowTicks, highTicks)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    if ((res = DAQmxCfgImplicitTiming(_taskHandle, DAQmx_Val_ContSamps, sampsPerChan)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    SendStatusMessage("NI Counter for triggering FLIm laser is successfully initialized.", false);

    return true;
}


void FlimTrigger::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is issueing external triggers for FLIm laser...", false);
        DAQmxStartTask(_taskHandle);
    }
}


void FlimTrigger::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void FlimTrigger::dumpError(int res, const char* pPreamble)
{	
    char errBuff[2048];
    if (res < 0)
        DAQmxGetErrorString(res, errBuff, 2048);

    //QMessageBox::critical(nullptr, "Error", (QString)pPreamble + (QString)errBuff);
    SendStatusMessage(((QString)pPreamble + (QString)errBuff).toUtf8().data(), true);

    if (_taskHandle)
    {
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}
