
#include "PulseTrainGenerator.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


PulseTrainGenerator::PulseTrainGenerator() :
	_taskHandle(nullptr),
	frequency(1000.0),
    finite_samps(100),
    counterChannel(""),    
	triggerSource("")
{
}


PulseTrainGenerator::~PulseTrainGenerator()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool PulseTrainGenerator::initialize()
{	
    SendStatusMessage("Initializing NI Counter for triggering with fixed frequency...", false);
		
    int res;

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    if ((res = DAQmxCreateCOPulseChanFreq(_taskHandle, counterChannel, "", DAQmx_Val_Hz, DAQmx_Val_Low, 0, frequency, 0.5)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

	if (triggerSource != "")
	{
		if ((res = DAQmxCfgDigEdgeStartTrig(_taskHandle, triggerSource, DAQmx_Val_Rising)) != 0)
		{
			dumpError(res, "ERROR: Failed to set NI Counter: ");
			return false;
		}
	}

    if ((res = DAQmxCfgImplicitTiming(_taskHandle, DAQmx_Val_ContSamps, finite_samps)) != 0) 
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    SendStatusMessage("NI Counter for triggering with fixed frequency is successfully initialized.", false);

    return true;
}


void PulseTrainGenerator::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is issueing external triggers with fixed frequency...", false);
        DAQmxStartTask(_taskHandle);
    }
}


void PulseTrainGenerator::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void PulseTrainGenerator::dumpError(int res, const char* pPreamble)
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
