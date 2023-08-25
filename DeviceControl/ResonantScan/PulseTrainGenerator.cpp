
#include "PulseTrainGenerator.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


PulseTrainGenerator::PulseTrainGenerator() :
    _taskHandle(nullptr),
    frequency(500),
    //counterChannel(NI_RESONANT_TRIG_CHANNEL),
	triggerSource(""),
    sourceTerminal("100MHzTimeBase")
{
}


PulseTrainGenerator::~PulseTrainGenerator()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool PulseTrainGenerator::initialize()
{	
    SendStatusMessage("Initializing NI Counter for triggering FLIm laser...", false);

	int slow = 100 * 1000 * 1000 / frequency;

    int lowTicks = (int)(ceil(slow / 2));
    int highTicks = (int)(floor(slow / 2));
    uint64_t sampsPerChan = 1'000'000;
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

	if (triggerSource != "")
	{
		if ((res = DAQmxCfgDigEdgeStartTrig(_taskHandle, triggerSource, DAQmx_Val_Rising))) //v sync generator 
		{
			dumpError(res, "ERROR: Failed to set NI Counter: ");
			return false;
		}
	}

    if ((res = DAQmxCfgImplicitTiming(_taskHandle, DAQmx_Val_ContSamps, sampsPerChan)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Counter: ");
        return false;
    }

    SendStatusMessage("NI Counter for triggering FLIm laser is successfully initialized.", false);

    return true;
}


void PulseTrainGenerator::start()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Counter is issueing external triggers for FLIm laser...", false);
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
