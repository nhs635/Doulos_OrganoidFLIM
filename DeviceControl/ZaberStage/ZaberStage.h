#ifndef ZABER_STAGE_H
#define ZABER_STAGE_H

#include <Common/callback.h>

#include "zb_serial.h"
#include "../QSerialComm.h";

#include <iostream>
#include <thread>


class ZaberStage
{
public:
	ZaberStage(const char* port_name, bool _new_device = false);
	~ZaberStage();

private:
	void SetCommandData(int cmd_data, uint8_t* msg);
	void GetReplyData(uint8_t* msg, int &reply_val);

public:
	bool ConnectStage();
	void DisconnectStage();
	void StopWaitThread();

	inline void setMaxMicroResolution(int max_micro_resol) { max_micro_resolution = max_micro_resol; }
	inline void setMicroResolution(int micro_resol) { micro_resolution = micro_resol; }
	inline void setMicroStepSize(double microstep) { microstep_size = microstep; }
	inline void setConversionFactor(double conversion) { conversion_factor = conversion; }

	inline bool getIsMoving() { return is_moving; }
	inline void setIsMoving(bool status) { is_moving = status; }

	double ConvertDataToMMPos(int data);
	int ConvertMMToDataPos(double mm);
	double ConvertDataToMMSpeed(int data);
	int ConvertMMToDataSpeed(double mms);

	void Home(int _stageNumber);
	void Stop(int _stageNumber);
	void MoveAbsoulte(int _stageNumber, double position);
	void MoveRelative(int _stageNumber, double position);
	void SetTargetSpeed(int _stageNumber, double speed);
	void MoveAbsoulte(int _stageNumber, int position);
	void MoveRelative(int _stageNumber, int position);
	void SetTargetSpeed(int _stageNumber, int speed);
	void GetPos(int _stageNumber);
	void GetMaxPos(int _stageNumber);

public:
    callback<void> DidMovedRelative;
	callback<void> DidMonitoring;
	callback2<int, int> DidGetPos;
	callback2<const char*, bool> SendStatusMessage;

private:
	const char* port_name;
	bool new_device;
	int max_micro_resolution;
	int micro_resolution;
	double microstep_size;
	double conversion_factor;

private:
	z_port port;
	QSerialComm* m_pSerialComm;
	int stage_number;	
	int comm_num;
	uint8_t home[6], stop[6], move_absolute[6], move_relative[6], target_speed[6], get_pos[6], get_max_pos[6], received_msg[6];

private:
	std::thread t_wait;
	bool _running_w;

	std::thread t_monitor;
	bool _running_m;

	bool is_moving;
};

#endif
