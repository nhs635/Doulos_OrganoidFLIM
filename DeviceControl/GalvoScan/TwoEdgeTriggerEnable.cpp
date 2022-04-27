
#include "TwoEdgeTriggerEnable.h"
#include <Doulos/Configuration.h>

#include <iostream>

#include <NIDAQmx.h>
using namespace std;


TwoEdgeTriggerEnable::TwoEdgeTriggerEnable() :
	_taskHandle(nullptr),
    lines(NI_GALVO_TWO_EDGE_LINE)
{
}


TwoEdgeTriggerEnable::~TwoEdgeTriggerEnable()
{
    if (_taskHandle)
        DAQmxClearTask(_taskHandle);
}


bool TwoEdgeTriggerEnable::initialize()
{	
    SendStatusMessage("Initializing NI Digital Input for detecting both rising and falling edges...", false);
		
    int res;

    if ((res = DAQmxCreateTask("", &_taskHandle)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Digital Input: ");
        return false;
    }

    if ((res = DAQmxCreateDIChan(_taskHandle, lines, "", DAQmx_Val_ChanPerLine)) != 0)
    {
        dumpError(res, "ERROR: Failed to set NI Digital Input: ");
        return false;
    }

	if ((res = DAQmxCfgChangeDetectionTiming(_taskHandle, lines, lines, DAQmx_Val_ContSamps, 100)) != 0)
	{
		dumpError(res, "ERROR: Failed to set NI Digital Input: ");
		return false;
	}
	
    SendStatusMessage("NI Digital Input for detecting both rising and falling edges is successfully initialized.", false);

    return true;
}


void TwoEdgeTriggerEnable::start()
{
    if (_taskHandle)
    {
		int res;
		uInt8 data[32];
		int32 numRead;
		int32 bytesPerSamp;

        SendStatusMessage("NI Digital Input enable to detect both rising and falling edges.", false);
        DAQmxStartTask(_taskHandle);

		if ((res = DAQmxReadDigitalLines(_taskHandle, 4, 10.0, DAQmx_Val_GroupByScanNumber, data, 32, &numRead, &bytesPerSamp, NULL)) != 0)
		{
			dumpError(res, "ERROR: Failed to set NI Digital Input: ");
			return;
		}
    }
}


void TwoEdgeTriggerEnable::stop()
{
    if (_taskHandle)
    {
        SendStatusMessage("NI Digital Input is stopped.", false);
        DAQmxStopTask(_taskHandle);
        DAQmxClearTask(_taskHandle);
        _taskHandle = nullptr;
    }
}


void TwoEdgeTriggerEnable::dumpError(int res, const char* pPreamble)
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
