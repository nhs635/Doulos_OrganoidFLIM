#ifndef NANOSCOPE_STAGE_H
#define NANOSCOPE_STAGE_H

#include <Common/callback.h>

#include "../QSerialComm.h"

enum stage_name
{
	x_stage = 0x01, 
	y_stage = 0x02,
	z_stage = 0x03
};

enum operation
{
	rel_cw = 0x01,  
	rel_ccw = 0x02,
	stop = 0x03,
	pos_que = 0x04,
	absol = 0x05,
	speed = 0x06	
};

class NanoscopeStage
{
public:
	explicit NanoscopeStage();
	virtual ~NanoscopeStage();

public:
	bool ConnectDevice();
	void DisconnectDevice();

	void writeStage(uint8_t stage, uint8_t operation, uint32_t data = 0);

public:
	inline void SetPortName(const char* _port_name) { port_name = _port_name; }
	inline bool getIsMoving() { return _moving; }

private:
	uint8_t calcCheckSum(char* command);

private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	bool _moving;
	char command[10] = { (char)0x00, (char)0x0B, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, (char)0x00, '\n' };
	
public:
	callback<bool> DidWriteCommand;
	callback2<uint8_t, double> DidGetCurPos;
	callback2<const char*, bool> SendStatusMessage;
};

#endif
