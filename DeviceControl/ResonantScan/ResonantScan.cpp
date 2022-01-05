
#include "ResonantScan.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


ResonantScan::ResonantScan() :
	_taskHandle(nullptr),
	physicalChannel(NI_RESONANT_CHANNEL) 
{
}

ResonantScan::~ResonantScan()
{
	if (_taskHandle) 
		DAQmxClearTask(_taskHandle);
}


bool ResonantScan::initialize()
{
	SendStatusMessage("Initializing NI Analog Output for resonant scanner control...", false);

	int res; 

	if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
	{
		dumpError(res, "ERROR: Failed to set resonant scanner control: ");
		return false;
	}
	if ((res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", 0.0, 5.0, DAQmx_Val_Volts, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set resonant scanner control: ");
		return false;
	}

	SendStatusMessage("NI Analog Output for resonant scanner control is successfully initialized.", false);

	return true;
}


bool ResonantScan::apply(double voltage)
{
	if (_taskHandle)
	{
		int res;
		
		if ((res = DAQmxStartTask(_taskHandle)))
		{
			dumpError(res, "ERROR: Failed to set resonant scanner control: ");
			return false;
		}

		if ((res = DAQmxWriteAnalogScalarF64(_taskHandle, FALSE, DAQmx_Val_WaitInfinitely, voltage, NULL)))
		{
			dumpError(res, "ERROR: Failed to set resonant scanner control: ");
			return false;
		}

		if ((res = DAQmxStopTask(_taskHandle)))
		{
			dumpError(res, "ERROR: Failed to set resonant scanner control: ");
			return false;
		}

		char msg[256];
		sprintf(msg, "A voltage is applied for resonant scanner control (%.2f V)...", voltage);
		SendStatusMessage(msg, false);
		
		return true;
	}

	return false;
}


void ResonantScan::dumpError(int res, const char* pPreamble)
{	
	char errBuff[2048];
	if (res < 0)
		DAQmxGetErrorString(res, errBuff, 2048);

	//QMessageBox::critical(nullptr, "Error", (QString)pPreamble + (QString)errBuff);

	char msg[256];
	sprintf(msg, "%s", ((QString)pPreamble + (QString)errBuff).toUtf8().data());
	SendStatusMessage(msg, true);

	if (_taskHandle)
	{
		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);
		_taskHandle = nullptr;
	}
}