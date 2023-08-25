
#include "DpcIllumination.h"


DpcIllumination::DpcIllumination() :
	port_name("")
{
	SetPortName("");
	m_pSerialComm = new QSerialComm;
}

DpcIllumination::~DpcIllumination()
{
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool DpcIllumination::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->getConnectState())
	{
		if (m_pSerialComm->openSerialPort(port_name, QSerialPort::Baud115200))
		{
			char msg[256];
			sprintf(msg, "[DPC ILLUMINATION] Success to connect to %s.", port_name);
			SendStatusMessage(msg, false);

			m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			{
				static char msg[256];
				static int j = 0;

				for (int i = 0; i < (int)len; i++)
				{
					msg[j++] = buffer[i];
					if (j > 255) j = 0;
				}

				if (msg[j - 1] == '\n')
				{
					// Send received message
					msg[j] = '\0';
					char send_msg[256];
					sprintf(send_msg, "[DPC ILLUMINATION] Receive: %s", msg);
					SendStatusMessage(send_msg, false);
					
					// Re-initialize
					j = 0;
				}
			};
		}
		else
		{
			char msg[256];
			sprintf(msg, "[DPC ILLUMINATION] Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("[DPC ILLUMINATION] Already connected.", false);
	
	return true;
}

void DpcIllumination::DisconnectDevice()
{
	if (m_pSerialComm->getConnectState())
	{
		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "[DPC ILLUMINATION] Success to disconnect from %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void DpcIllumination::setIlluminationPattern(int pattern)
{
	if (m_pSerialComm)
	{
		switch (pattern)
		{
		case off:
			m_pSerialComm->writeSerialPort((char*)"g", 2);
			break;
		case brightfield:
			m_pSerialComm->writeSerialPort((char*)"f", 2);
			break;
		case darkfield:
			m_pSerialComm->writeSerialPort((char*)"l", 2);
			break;
		case illum_top:
			m_pSerialComm->writeSerialPort((char*)"d", 2);
			break;
		case illum_bottom:
			m_pSerialComm->writeSerialPort((char*)"c", 2);
			break;
		case illum_left:
			m_pSerialComm->writeSerialPort((char*)"a", 2);
			break;
		case illum_right:
			m_pSerialComm->writeSerialPort((char*)"b", 2);
			break;
		default:
			m_pSerialComm->writeSerialPort((char*)" ", 2);
			break;
		}
		
#if _DEBUG
		m_pSerialComm->waitUntilResponse(50);
#else
		m_pSerialComm->waitUntilResponse(50);
#endif
	}
}
