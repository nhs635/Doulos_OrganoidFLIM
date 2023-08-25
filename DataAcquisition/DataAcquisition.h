#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Doulos/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>

class AlazarDAQ;
class FLImProcess;
class QpiProcess;
class ImagingSource;


class DataAcquisition : public QObject
{
	Q_OBJECT

public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

public:
    inline FLImProcess* getFLIm() const { return m_pFLIm; }
	inline QpiProcess* getQpi() const { return m_pQpi; }
	inline ImagingSource* getImagingSource() const { return m_pImagingSource; }

public:
    bool InitializeAcquistion(bool is_flim = true);
    bool StartAcquisition(bool is_flim = true);
    void StopAcquisition(bool is_flim = true);
		
public:
	void ConnectDaqAcquiredFlimData(const std::function<void(int, const void*)> &slot);
    void ConnectDaqStopFlimData(const std::function<void(void)> &slot);
    void ConnectDaqSendStatusMessage(const std::function<void(const char*, bool)> &slot);
	void ConnectBrightfieldAcquiredFlimData(const std::function<void(int, const np::Uint16Array2 &)> &slot);
	void ConnectBrightfieldStopFlimData(const std::function<void(void)> &slot);
	void ConnectBrightfieldSendStatusMessage(const std::function<void(const char*, bool)> &slot);

private:
	Configuration* m_pConfig;

	AlazarDAQ* m_pDaq;
    FLImProcess* m_pFLIm;
	QpiProcess* m_pQpi;
	ImagingSource* m_pImagingSource;
};

#endif // DATAACQUISITION_H
