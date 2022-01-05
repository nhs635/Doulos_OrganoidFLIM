#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Doulos/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>

class AlazarDAQ;
class FLImProcess;


class DataAcquisition : public QObject
{
	Q_OBJECT

public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

public:
    inline FLImProcess* getFLIm() const { return m_pFLIm; }

public:
    bool InitializeAcquistion();
    bool StartAcquisition();
    void StopAcquisition();
		
public:
	void ConnectDaqAcquiredFlimData(const std::function<void(int, const void*)> &slot);
    void ConnectDaqStopFlimData(const std::function<void(void)> &slot);
    void ConnectDaqSendStatusMessage(const std::function<void(const char*, bool)> &slot);

private:
	Configuration* m_pConfig;

	AlazarDAQ* m_pDaq;
    FLImProcess* m_pFLIm;
};

#endif // DATAACQUISITION_H
