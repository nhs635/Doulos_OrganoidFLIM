
#include "ZaberStage.h"
#include <Doulos/Configuration.h>


ZaberStage::ZaberStage(const char* _port_name, bool _new_device) :
	port_name(_port_name),
	new_device(_new_device),
	max_micro_resolution(256),
	micro_resolution(256),
	conversion_factor(1 / 9.375),
	microstep_size(0.099),
	port(INVALID_HANDLE_VALUE),
	m_pSerialComm(nullptr),
	stage_number(1),
	comm_num(1),	
	_running_w(false), _running_m(false),
	is_moving(false)
{
	memset(home, 0, sizeof(home));
	memset(stop, 0, sizeof(home));
	memset(target_speed, 0, sizeof(home));
	memset(move_absolute, 0, sizeof(home));

	if (new_device)
	{
		m_pSerialComm = new QSerialComm;
		m_pSerialComm->setPortName(port_name);
	}
}


ZaberStage::~ZaberStage()
{
	if (!new_device)
	{
		if (t_wait.joinable())
		{
			_running_w = false;
			t_wait.join();
		}
	}
	else
	{
		if (t_monitor.joinable())
		{
			_running_m = false;
			t_monitor.join();
		}
	}

	DisconnectStage();
	if (new_device)
		if (m_pSerialComm)
			delete m_pSerialComm;
}


void ZaberStage::SetCommandData(int cmd_data, uint8_t* msg)
{
	unsigned int power24 = 1 << (8 * 3);
	unsigned int power16 = 1 << (8 * 2);
	unsigned int power08 = 1 << (8 * 1);

	int c3, c4, c5, c6;

	c6 = cmd_data / power24;
	cmd_data = cmd_data - power24 * c6;
	c5 = cmd_data / power16;
	cmd_data = cmd_data - power16 * c5;
	c4 = cmd_data / power08;
	cmd_data = cmd_data - power08 * c4;
	c3 = cmd_data;

	msg[2] = c3;
	msg[3] = c4;
	msg[4] = c5;
	msg[5] = c6;
}


void ZaberStage::GetReplyData(uint8_t* msg, int &reply_val)
{
	unsigned int power24 = 1 << (8 * 3);
	unsigned int power16 = 1 << (8 * 2);
	unsigned int power08 = 1 << (8 * 1);

	int c3, c4, c5, c6;
	
	c3 = msg[2];
	c4 = msg[3];
	c5 = msg[4];
	c6 = msg[5];

	reply_val = power24 * c6 + power16 * c5 + power08 * c4 + c3;
	if (c6 > 127)
		reply_val = 0; // (unsigned long long)reply_val - (unsigned long long)(256 * 256 * 256 * 256);
}


bool ZaberStage::ConnectStage()
{
	if (!new_device)
	{
		if (port == INVALID_HANDLE_VALUE)
		{
			SendStatusMessage("ZABER: Connecting to Zaber stage...", false);

			if (zb_connect(&port, port_name) != Z_SUCCESS)
			{
				char msg[256];
				sprintf(msg, "ZABER: Could not connect to device %s.", port_name);
				SendStatusMessage(msg, true);
				return false;
			}
			home[0] = 1; stop[0] = 1; target_speed[0] = 1; move_absolute[0] = 1; move_relative[0] = 1; get_pos[0] = 1; get_max_pos[0] = 1;
			home[1] = 1; stop[1] = 23; target_speed[1] = 42; move_absolute[1] = 20; move_relative[1] = 21; get_pos[1] = 60; get_max_pos[1] = 44;

			SendStatusMessage("ZABER: Success to connect to Zaber stage...", false);
		}

		// Message receiving thread
		if (!(t_wait.joinable()))
		{
			t_wait = std::thread([&]() {

				DWORD nread;
				int i = 0; char c;

				_running_w = true;
				while (_running_w)
				{
					if (!_running_w)
						break;

					// Read message byte by byte
					ReadFile(port, &c, 1, &nread, NULL);

					// Writing message
					if ((nread != 0))
						received_msg[i++] = c;

					// Show message
					if (i == 6)
					{
						char msg[256];
						sprintf(msg, "ZABER: Received [%d %d %d %d %d %d].", received_msg[0], received_msg[1]
							, received_msg[2], received_msg[3]
							, received_msg[4], received_msg[5]);
						SendStatusMessage(msg, false);
						i = 0;

						if ((received_msg[1] == 21) || (received_msg[1] == 20))
						{
							SendStatusMessage("Moved relatively.", false);
							DidMovedRelative();
							GetPos(received_msg[0]);
						}

						if ((received_msg[1] == 23) || (received_msg[1] == 1))
						{
							GetPos(received_msg[0]);
						}

						if ((received_msg[1] == 60) || (received_msg[1] == 44))
						{
							int pos = 0;
							for (int jj = 0; jj < 4; jj++)
								pos += received_msg[jj + 2] << (8 * jj);

							DidGetPos(100 * received_msg[0] + received_msg[1], pos);
						}
					}
				}
			});
		}
		
		// Set micro resolution
		uint8_t resol[6] = { 0, }; resol[1] = 37;

		resol[0] = 1; SetCommandData(micro_resolution, resol);
		zb_send(port, resol);
		resol[0] = 2; SetCommandData(micro_resolution, resol);
		zb_send(port, resol);

		char msg[256];
		sprintf(msg, "ZABER: Set micro resolution step %d!!.", micro_resolution);
		SendStatusMessage(msg, false);
	}
	else
	{
		// Open a port
		if (!m_pSerialComm->getConnectState())
		{
			if (m_pSerialComm->openSerialPort(m_pSerialComm->getPortName(), QSerialPort::Baud115200))
			{
				printf("ZABER: Success to connect to %s.\n", m_pSerialComm->getPortName());

				m_pSerialComm->DidReadBuffer += [&](char* buffer, qint64 len)
				{
					static char msg[256];
					static int j = 0;

					for (int i = 0; i < (int)len; i++)
					{
						msg[j++] = buffer[i];

						if (buffer[i] == '\n')
						{
							msg[j] = '\0';
							SendStatusMessage(msg, false);
							
							if (is_moving)
							{
								if (msg[12] == 'I')
								{
									is_moving = false;
									int cur_pos = atoi(&msg[20]);
									DidGetPos(3, cur_pos);
								}
							}
							//if (is_moving) DidMovedRelative(msg);
							j = 0;
						}

						if (j == 256) j = 0;
					}


					//for (int i = 0; i < (int)len; i++)
					//{
					//	printf("%c", buffer[i]);
					//}
					////int data = 0;

					//printf("\n");

					//// get position data
					//int data = atoi(buffer + ((int)len - 8));
					//DidGetPos(3, data);
					////double ddata = data * MICROSTEP_SIZE / 1000;

					//////Zposition = ddata;

					////printf("%f\n", ddata);

					//// DidFinishMoving(msg);
				};


				// Define and execute current pos monitoring thread
				if (!(t_monitor.joinable()))
				{
					t_monitor = std::thread([&]() {
						_running_m = true;
						while (_running_m)
						{
							if (!_running_m)
								break;

							std::this_thread::sleep_for(std::chrono::milliseconds(200));
							if (is_moving) DidMonitoring();
						}
					});
				}
			}
			else
			{
				char msg[256];
				sprintf(msg, "ZABER: Fail to connect to %s.\n", m_pSerialComm->getPortName());
				SendStatusMessage(msg, false);

				return false;
			}
		}
		else
			SendStatusMessage("ZABER: Already connected.", false);
	}

	return true;
}


void ZaberStage::DisconnectStage()
{
	if (!new_device)
	{
		if (port != INVALID_HANDLE_VALUE)
		{
			Stop(1);
			zb_disconnect(port);
			port = INVALID_HANDLE_VALUE;
			SendStatusMessage("ZABER: Success to disconnect to Zaber stage...", false);
		}
	}
	else
	{
		if (m_pSerialComm->getConnectState())
		{
			Stop(1);
			m_pSerialComm->closeSerialPort();

			char msg[256];
			sprintf(msg, "ZABER: Success to disconnect to %s.", m_pSerialComm->getPortName());
			SendStatusMessage(msg, false);
		}
	}
}


void ZaberStage::StopWaitThread()
{
	if (t_wait.joinable())
	{
		_running_w = false;
		t_wait.join();
	}
}

double ZaberStage::ConvertDataToMMPos(int data)
{
	if (!new_device)
		return (double)data / 1000.0 * (microstep_size * (double)max_micro_resolution / (double)micro_resolution);
	else
		return (double)data / 1000.0 * microstep_size;
}

int ZaberStage::ConvertMMToDataPos(double mm)
{
	if (!new_device)
		return (int)round(1000.0 * mm / (microstep_size * (double)max_micro_resolution / (double)micro_resolution));
	else
		return (int)round(1000.0 * mm / microstep_size);

}

double ZaberStage::ConvertDataToMMSpeed(int data)
{
	if (!new_device)
		return (double)data / 1000.0 / conversion_factor * (microstep_size * (double)max_micro_resolution / (double)micro_resolution);
	else
		return (double)data / 1000.0 / conversion_factor * microstep_size;
}

int ZaberStage::ConvertMMToDataSpeed(double mms)
{
	if (!new_device)
		return (int)round(1000.0 * mms * conversion_factor / (microstep_size * (double)max_micro_resolution / (double)micro_resolution));
	else
		return (int)round(1000.0 * mms * conversion_factor / microstep_size);
}


void ZaberStage::Home(int _stageNumber)
{
	if (!new_device)
	{
		home[0] = _stageNumber; zb_send(port, home);		
		SendStatusMessage("ZABER: Go home!!", false);
	}
	else
	{
		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d home\n", _stageNumber, comm_num);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}

	is_moving = true;
}


void ZaberStage::Stop(int _stageNumber)
{
	if (!new_device)
	{
		stop[0] = _stageNumber; zb_send(port, stop);
		SendStatusMessage("ZABER: Halted the operation intentionally.", false);

		is_moving = false;
	}
	else
	{
		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d stop\n", _stageNumber, comm_num);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}
}


void ZaberStage::MoveAbsoulte(int _stageNumber, double position)
{	
	if (!new_device)
	{
		move_absolute[0] = _stageNumber;

		int cmd_abs_dist = ConvertMMToDataPos(position);
		SetCommandData(cmd_abs_dist, move_absolute);
		zb_send(port, move_absolute);

		char msg[256];
		sprintf(msg, "ZABER: Move absolute %d (%.1f mm)", cmd_abs_dist, position);
		SendStatusMessage(msg, false);
	}
	else
	{
		int data = ConvertMMToDataPos(position);

		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d move abs %d\n", _stageNumber, comm_num, data);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}

	is_moving = true;
}


void ZaberStage::MoveRelative(int _stageNumber, double position)
{
	if (!new_device)
	{
		move_relative[0] = _stageNumber;

		int cmd_rel_dist = ConvertMMToDataPos(position);
		SetCommandData(cmd_rel_dist, move_relative);
		zb_send(port, move_relative);

		char msg[256];
		sprintf(msg, "ZABER: Move relative %d (%.1f mm)", cmd_rel_dist, position);
		SendStatusMessage(msg, false);
	}
	else
	{
		int data = ConvertMMToDataPos(position);

		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d move rel %d\n", _stageNumber, comm_num, data);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;
				
		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}

	is_moving = true;
}


void ZaberStage::SetTargetSpeed(int _stageNumber, double speed)
{
	if (!new_device)
	{
		target_speed[0] = _stageNumber;

		int cmd_speed = ConvertMMToDataSpeed(speed);
		SetCommandData(cmd_speed, target_speed);
		zb_send(port, target_speed);

		char msg[256];
		sprintf(msg, "ZABER: Set target speed %d (%.1f mm/sec)", cmd_speed, speed);
		SendStatusMessage(msg, false);
	}
	else
	{
		int data = ConvertMMToDataSpeed(speed);

		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d set maxspeed %d\n", _stageNumber, comm_num, data);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}
}


void ZaberStage::MoveAbsoulte(int _stageNumber, int position)
{
	if (!new_device)
	{
		move_absolute[0] = _stageNumber;

		SetCommandData(position, move_absolute);
		zb_send(port, move_absolute);

		char msg[256];
		sprintf(msg, "ZABER: Move absolute %d (%.1f mm)", position, ConvertDataToMMPos(position));
		SendStatusMessage(msg, false);
	}
	else
	{
		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d move abs %d\n", _stageNumber, comm_num, position);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}

	is_moving = true;
}


void ZaberStage::MoveRelative(int _stageNumber, int position)
{
	if (!new_device)
	{
		move_relative[0] = _stageNumber;

		SetCommandData(position, move_relative);
		zb_send(port, move_relative);

		char msg[256];
		sprintf(msg, "ZABER: Move relative %d (%.1f mm)", position, ConvertDataToMMPos(position));
		SendStatusMessage(msg, false);
	}
	else
	{
		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d move rel %d\n", _stageNumber, comm_num, position);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}

	is_moving = true;
}


void ZaberStage::SetTargetSpeed(int _stageNumber, int speed)
{
	if (!new_device)
	{
		target_speed[0] = _stageNumber;

		SetCommandData(speed, target_speed);
		zb_send(port, target_speed);

		char msg[256];
		sprintf(msg, "ZABER: Set target speed %d (%.1f mm/sec)", speed, ConvertDataToMMSpeed(speed));
		SendStatusMessage(msg, false);
	}
	else
	{
		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d set maxspeed %d\n", _stageNumber, comm_num, speed);
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}
}


void ZaberStage::GetPos(int _stageNumber)
{
	if (!new_device)
	{
		get_pos[0] = _stageNumber; zb_send(port, get_pos);
		SendStatusMessage("ZABER: Get pos", false);
	}
	else
	{
		char buff[100];
		sprintf_s(buff, sizeof(buff), "/%02d 0 %02d get pos\n", 1, comm_num);		
		comm_num = (comm_num != 96) ? comm_num + 1 : 0;

		char msg[256];
		sprintf(msg, "ZABER: Send: %s", buff);
		SendStatusMessage(msg, false);

		m_pSerialComm->writeSerialPort(buff);
	}
}


void ZaberStage::GetMaxPos(int _stageNumber)
{
	//if (!new_device)
	//{
	//	get_max_pos[0] = _stageNumber; zb_send(port, get_max_pos);
	//	SendStatusMessage("ZABER: Get max pos", false);
	//}
	//else
	//{
	//	char buff[100];
	//	sprintf_s(buff, sizeof(buff), "/%02d 0 %02d get pos\n", 1, comm_num);
	//	printf("%02d 0 %02d get pos\n", _stageNumber, comm_num);
	//	comm_num = (comm_num != 96) ? comm_num + 1 : 0;

	//	char msg[256];
	//	sprintf(msg, "ZABER: Send: %s", buff);
	//	SendStatusMessage(msg, false);

	//	m_pSerialComm->writeSerialPort(buff);
	//}
}