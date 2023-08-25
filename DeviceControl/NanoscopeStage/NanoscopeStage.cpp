
#include "NanoscopeStage.h"


NanoscopeStage::NanoscopeStage() :
	port_name(""), _moving(false)
{
	SetPortName("");
	m_pSerialComm = new QSerialComm;
}

NanoscopeStage::~NanoscopeStage()
{
	DisconnectDevice();
	if (m_pSerialComm) delete m_pSerialComm;
}


bool NanoscopeStage::ConnectDevice()
{
	// Open a port
	if (!m_pSerialComm->getConnectState())
	{
		if (m_pSerialComm->openSerialPort(port_name, QSerialPort::Baud115200))
		{
			char msg[256];
			sprintf(msg, "[NANOSCOPE] Success to connect to %s.", port_name);
			SendStatusMessage(msg, false);

			m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
			{
				static char msg[256];
				static int j = 0;

				for (int i = 0; i < (int)len; i++)
				{
					msg[j++] = buffer[i];

					if (j == 9)
					{
						// Send received message
						msg[j] = '\0';

						char send_msg[256];
						sprintf(send_msg, "[NANOSCOPE] Receive: %02X %02X %02X %02X  %02X %02X %02X %02X  %02X",
							(unsigned char)msg[0], (unsigned char)msg[1], (unsigned char)msg[2],
							(unsigned char)msg[3], (unsigned char)msg[4], (unsigned char)msg[5],
							(unsigned char)msg[6], (unsigned char)msg[7], (unsigned char)msg[8]);
						SendStatusMessage(send_msg, false);

						// Get current position
						if ((msg[0] == 0x13) && (msg[3] == 0x04))
						{
							uint32_t _cur_pos = 0;
							for (int i = 0; i < 4; i++)
								_cur_pos += ((uint32_t)msg[4 + i] << (8 * (3 - i))) & (0xFF << (8 * (3 - i)));

							double cur_pos = (double)_cur_pos / 1'000'000.0;
							DidGetCurPos(msg[2], cur_pos);
						}

						// Request current position when success signal is obtained
						if ((msg[0] == 0x17))
						{
							_moving = false;
							writeStage(msg[2], pos_que);
						}

						// Re-initialize
						j = 0;

						break;
					}

					if (j > 255) j = 0;
				}
			};
		}
		else
		{
			char msg[256];
			sprintf(msg, "[NANOSCOPE] Fail to connect to %s.", port_name);
			SendStatusMessage(msg, false);
			return false;
		}
	}
	else
		SendStatusMessage("[NANOSCOPE] Already connected.", false);

	writeStage(x_stage, pos_que);
	writeStage(y_stage, pos_que);
	writeStage(z_stage, pos_que);

	return true;
}

void NanoscopeStage::DisconnectDevice()
{
	if (m_pSerialComm->getConnectState())
	{
		m_pSerialComm->closeSerialPort();

		char msg[256];
		sprintf(msg, "[NANOSCOPE] Success to disconnect from %s.", port_name);
		SendStatusMessage(msg, false);
	}
}


void NanoscopeStage::writeStage(uint8_t stage, uint8_t operation, uint32_t data)
{
	command[0] = 0x11;
	command[2] = stage;
	command[3] = operation;
	if (operation == pos_que) command[0] = 0x21;

	for (int i = 0; i < 4; i++)
		command[7 - i] = (uint8_t)(0xFF & (data >> (8 * i)));
	command[8] = calcCheckSum(command);

	char msg[256];
	sprintf(msg, "[NANOSCOPE] Send: %02X %02X %02X %02X  %02X %02X %02X %02X  %02X",
		(unsigned char)command[0], (unsigned char)command[1], (unsigned char)command[2],
		(unsigned char)command[3], (unsigned char)command[4], (unsigned char)command[5],
		(unsigned char)command[6], (unsigned char)command[7], (unsigned char)command[8]);
	SendStatusMessage(msg, false);

	if (m_pSerialComm)
	{
		m_pSerialComm->writeSerialPort(command, 10);
		m_pSerialComm->waitUntilResponse();
	}

	if ((operation == rel_ccw) || (operation == rel_cw))
		_moving = true;
	else if (operation == stop)
		_moving = false;	

	DidWriteCommand(_moving);
}

//void NanoscopeStage::readStage(uint8_t stage)
//{
//   command[0] = 0x11;
//   command[1] = stage;
//   command[2] = operation;
//   for (int i = 0; i < 4; i++)
//      command[3 + 0] = (uint8_t)(0xFF & (data >> 8 * i));
//   command[7] = calcCheckSum(command);
//
//   char msg[256];
//   sprintf(msg, "[NANOSCOPE] Send: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
//      (unsigned char)command[0], (unsigned char)command[1], (unsigned char)command[2],
//      (unsigned char)command[3], (unsigned char)command[4], (unsigned char)command[5],
//      (unsigned char)command[6], (unsigned char)command[7], (unsigned char)command[8]);
//   SendStatusMessage(msg, false);
//
//   m_pSerialComm->writeSerialPort(command);
//   m_pSerialComm->waitUntilResponse();
//}


uint8_t NanoscopeStage::calcCheckSum(char* command)
{
	uint16_t checksum = 0;
	for (uint8_t i = 0; i < 8; i++)
		checksum += (uint16_t)command[i];

	return (uint8_t)(checksum & 0x00FF);
}