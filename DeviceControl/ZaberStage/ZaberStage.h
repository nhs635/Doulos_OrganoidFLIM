#ifndef ZABER_STAGE_H
#define ZABER_STAGE_H

#include <Common/callback.h>

#include "zb_serial.h"

#include <iostream>
#include <thread>


class ZaberStage
{
public:
	ZaberStage();
	~ZaberStage();

private:
	void SetCommandData(int cmd_data, uint8_t* msg);
	void GetReplyData(uint8_t* msg, int &reply_val);

public:
	bool ConnectStage();
	void DisconnectStage();
	void StopWaitThread();

	void Home(int _stageNumber);
	void Stop();
	void MoveAbsoulte(int _stageNumber, double position);
	void MoveRelative(int _stageNumber, double position);
	void SetTargetSpeed(int _stageNumber, double speed);

public:
    callback<void> DidMovedRelative;
	callback2<const char*, bool> SendStatusMessage;

private:
	const char* device_name;
	int max_micro_resolution;
	int micro_resolution;
	double conversion_factor;

private:
	z_port port;
	int stage_number;
	double microstep_size;
	uint8_t home[6], stop[6], move_absolute[6], move_relative[6], target_speed[6], received_msg[6];

private:
	std::thread t_wait;
	bool _running;
};

#endif
