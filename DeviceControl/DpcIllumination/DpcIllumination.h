#ifndef DPC_ILLUMINATION_H
#define DPC_ILLUMINATION_H

#include <Common/callback.h>

#include "../QSerialComm.h"

enum illumination
{
	off = 0,
	brightfield, darkfield,
	illum_top, illum_bottom, illum_left, illum_right
};

class DpcIllumination
{
public:
	explicit DpcIllumination();
	virtual ~DpcIllumination();

public:
	bool ConnectDevice();
	void DisconnectDevice();

	void setIlluminationPattern(int pattern = 0);

public:
	inline void SetPortName(const char* _port_name) { port_name = _port_name; }
	
private:
	QSerialComm* m_pSerialComm;
	const char* port_name;
	
public:
	callback2<const char*, bool> SendStatusMessage;
};

#endif
