
#include "PulseTrainDivider.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


PulseTrainDivider::PulseTrainDivider() :
	_taskHandle(nullptr),
	slow(4),    
    counterChannel(""),    
	sourceTerminal("")
{
}


PulseTrainDivider::~PulseTrainDivider()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool PulseTrainDivider::initialize()
{	
    SendStatusMessage("Initializing NI Counter for dividing the input trigger...", false);
		
	int lowTicks = (int)(ceil(slow / 2));
	int highTicks = (int)(floor(slow / 2));
	uint64_t sampsPerChan = 1100000;
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

    SendStatusMessage("NI Counter for dividing the input trigger is successfully initialized.", false);

    return true;
}


void PulseTrainDivider::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is issueing divided pulse trigger...", false);
        DAQmxStartTask(_taskHandle);
    }
}


void PulseTrainDivider::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void PulseTrainDivider::dumpError(int res, const char* pPreamble)
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
