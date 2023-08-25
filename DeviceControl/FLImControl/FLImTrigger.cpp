
#include "FlimTrigger.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


FlimTrigger::FlimTrigger() :
	_taskHandle(nullptr),
	frequency(1000.0),
    finite_samps(1),
    counterChannel(NI_FLIM_TRIG_CHANNEL),
    sourceTerminal(NI_FLIM_TRIG_SOURCE),
	triggerSource("")
{
}


FlimTrigger::~FlimTrigger()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool FlimTrigger::initialize()
{	
    SendStatusMessage("Initializing NI Counter for triggering FLIm laser & digitizer...", false);
		
    int res;

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    if ((res = DAQmxCreateCOPulseChanFreq(_taskHandle, counterChannel, "", DAQmx_Val_Hz, DAQmx_Val_Low, 0, frequency, 0.50)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

	if ((res = DAQmxCfgDigEdgeStartTrig(_taskHandle, sourceTerminal, DAQmx_Val_Rising)) != 0)
	{
		dumpError(res, "ERROR: Failed to set NI Counter: ");
		return false;
	}

    if ((res = DAQmxCfgImplicitTiming(_taskHandle, DAQmx_Val_FiniteSamps, finite_samps)) != 0) //DAQmx_Val_FiniteSamps
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

	if ((res = DAQmxSetStartTrigRetriggerable(_taskHandle, TRUE)) != 0)
	{
		dumpError(res, "ERROR: Failed to set NI Counter: ");
		return false;
	}

    SendStatusMessage("NI Counter for triggering FLIm laser & digitizer is successfully initialized.", false);

    return true;
}


void FlimTrigger::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is issueing external triggers for FLIm laser & digitizer...", false);
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
