
#include "GalvoScan.h"
#include <Doulos/Configuration.h>

#include <iostream>
#include <fstream>

#include <NIDAQmx.h>
using namespace std;


GalvoScan::GalvoScan() :
	_taskHandle(nullptr),
	freq_fast(500.0),
	pp_voltage_fast(2.0),
	pp_voltage_slow(2.0),
	offset_fast(0.0),
	offset_slow(0.0),
	step(1000),
    max_rate(2000.0),
	data(nullptr),
	physicalChannel(NI_GALVO_CHANNEL),
    sourceTerminal(NI_GALVO_SOURCE)//,	
	//triggerSource(NI_GALVO_START_TRIG_SOURCE)
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

	// Scan pattern
	data = new double[2 * step];

	for (int i = 0; i < step; i++)
	{
		double t = (double)i / max_rate * freq_fast;
		double fast = (-4 * abs((t - floor(t)) - 0.5) + 1);
		data[2 * i] = pp_voltage_fast / 2 * fast + offset_fast;

		double slow = 2 * floor(2 * t) / (2 * freq_fast / max_rate * step - 1) - 1;
		data[2 * i + 1] = pp_voltage_slow / 2 * slow + offset_slow;
	}

	std::ofstream fout;
	fout.open("scan_pattern.dat", std::ios::out | std::ios::binary);

	if (fout.is_open()) {
		fout.write((const char*)data, sizeof(double) * 2 * step);
		fout.close();
	}

	//m_pGalvoScan[0]->step = FAST_DIR_FACTOR * N_PIXELS;
	//double* fast_scan = new double[m_pGalvoScan[0]->step];
	//for (int i = 0; i < 1000 * m_pGalvoScan[0]->step; i++)
	//{
	//	
	//	double fast = (-4 * abs((t - floor(t)) - 0.5) + 1);
	//	fast_scan[i] = m_pGalvoScan[0]->pp_voltage / 2 * fast + m_pGalvoScan[0]->offset;
	//}

	//// Slow scan pattern set-up
	//m_pGalvoScan[1]->step = (m_pConfig->nLines + GALVO_FLYING_BACK + 2);
	//double* slow_scan = new double[m_pGalvoScan[1]->step];
	//for (int i = 0; i < GALVO_FLYING_BACK; i++)
		//slow_scan[i] = -(double)i * (m_pGalvoScan[1]->pp_voltage * 2 / (GALVO_FLYING_BACK - 1)) + m_pGalvoScan[1]->pp_voltage + m_pGalvoScan[1]->offset;
	//for (int i = GALVO_FLYING_BACK; i < m_pGalvoScan[1]->step; i++)
		//slow_scan[i] = (double)(i + 1 - GALVO_FLYING_BACK) * (m_pGalvoScan[1]->pp_voltage * 2 / (m_pGalvoScan[1]->step - 1 - GALVO_FLYING_BACK)) - m_pGalvoScan[1]->pp_voltage + m_pGalvoScan[1]->offset;

	
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
	{
		dumpError(res, "ERROR: Failed to set galvoscanner3: ");
		return false;
	}
	//if ((res = DAQmxCfgDigEdgeStartTrig(_taskHandle, triggerSource, DAQmx_Val_Rising))) //v sync generator 
	//{
	//	dumpError(res, "ERROR: Failed to set galvoscanner4: ");
	//	return false;
	//}
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

