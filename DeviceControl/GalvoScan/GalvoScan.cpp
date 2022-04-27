
#include "GalvoScan.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


GalvoScan::GalvoScan() :
	_taskHandle(nullptr),
	pp_voltage(2.0),
	offset(0.0),
	step(1000),
    max_rate(2000.0),
	data(nullptr),
	physicalChannel(NI_GALVO_CHANNEL),
    sourceTerminal(NI_GALVO_SOURCE),
	
	triggerSource(NI_GALVO_START_TRIG_SOURCE)
{
}


GalvoScan::~GalvoScan()
{
	if (data)
	{
		delete[] data;
		data = nullptr;
	}
	if (_taskHandle) 
		DAQmxClearTask(_taskHandle);
}


bool GalvoScan::initialize()
{
    SendStatusMessage("Initializing NI Analog Output for galvano mirror...", false);

	int res;	
    int sample_mode = DAQmx_Val_ContSamps;

	// Bi-directional slow scan
	//data = new double[2 * step];	
	//for (int i = 0; i < step; i++)
	//{
	//	data[i] = (double)i * (pp_voltage * 2 / (step - 1)) - pp_voltage + offset;
	//	data[2 * step - 1 - i] = data[i];
	//}
	//step = 2 * step;
	//	
	// Uni-directional slow scan pattern
	data = new double[step];
	for (int i = 0; i < GALVO_FLYING_BACK; i++)
		data[i] = -(double)i * (pp_voltage * 2 / (GALVO_FLYING_BACK - 1)) + pp_voltage + offset;
	for (int i = GALVO_FLYING_BACK; i < step; i++)
		data[i] = (double)(i + 1 - GALVO_FLYING_BACK) * (pp_voltage * 2 / (step - 1 - GALVO_FLYING_BACK)) - pp_voltage + offset;
	
	/*********************************************/
	// Scan Part
	/*********************************************/
	if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner1: ");
		return false;
	}
	if ((res = DAQmxCreateAOVoltageChan(_taskHandle, physicalChannel, "", -10.0, 10.0, DAQmx_Val_Volts, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner2: ");
		return false;
	}
	if ((res = DAQmxCfgSampClkTiming(_taskHandle, sourceTerminal, max_rate, DAQmx_Val_Rising, sample_mode, step)) != 0)
	//if(res = DAQmxCfgChangeDetectionTiming(_taskHandle, "/Dev1 /PFI7", "/Dev1 /P0.16", sample_mode, step) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner3: ");
		return false;
	}
	if ((res = DAQmxCfgDigEdgeStartTrig(_taskHandle, triggerSource, DAQmx_Val_Rising))) //v sync generator 
	{
		dumpError(res, "ERROR: Failed to set galvoscanner4: ");
		return false;
	}
	if ((res = DAQmxWriteAnalogF64(_taskHandle, step, FALSE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByScanNumber, data, NULL, NULL)) != 0)
	{
		dumpError(res, "ERROR: Failed to set galvoscanner5: ");
		return false;
    }


    SendStatusMessage("NI Analog Output for galvano mirror is successfully initialized.", false);

	return true;
}


void GalvoScan::start()
{
	if (_taskHandle)
	{
        SendStatusMessage("Galvano mirror is scanning a sample...", false);
		DAQmxStartTask(_taskHandle);
	}
}


void GalvoScan::stop()
{
	if (_taskHandle)
	{
        SendStatusMessage("NI Analog Output is stopped.", false);
		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);
		if (data)
		{
			delete[] data;
			data = nullptr;
		}
		_taskHandle = nullptr;
	}
}


void GalvoScan::dumpError(int res, const char* pPreamble)
{	
	char errBuff[2048];
	if (res < 0)
		DAQmxGetErrorString(res, errBuff, 2048);

	//QMessageBox::critical(nullptr, "Error", (QString)pPreamble + (QString)errBuff);
    SendStatusMessage(((QString)pPreamble + (QString)errBuff).toUtf8().data(), true);

	if (_taskHandle)
	{
		double data[1] = { 0.0 };
		DAQmxWriteAnalogF64(_taskHandle, 1, TRUE, DAQmx_Val_WaitInfinitely, DAQmx_Val_GroupByChannel, data, NULL, NULL);

		DAQmxStopTask(_taskHandle);
		DAQmxClearTask(_taskHandle);

		_taskHandle = nullptr;
	}
}

