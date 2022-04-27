#ifndef MEMORYBUFFER_H
#define MEMORYBUFFER_H

#include <QObject>

#include <iostream>
#include <thread>
#include <queue>
#include <vector>

#include <Common/SyncObject.h>
#include <Common/callback.h>

class MainWindow;
class Configuration;
class QOperationTab;
class QDeviceControlTab;

class MemoryBuffer : public QObject
{
	Q_OBJECT

public:
    explicit MemoryBuffer(QObject *parent = nullptr);
    virtual ~MemoryBuffer();


public:
	// Memory allocation function (buffer for writing)
    void allocateWritingBuffer();
	void deallocateWritingBuffer();

    // Data recording (transfer streaming data to writing buffer)
    bool startRecording();
    void stopRecording();

    // Data saving (save wrote data to hard disk)
    bool startSaving();
	
private: // writing threading operation
	void write();

public:
	// General inline function
	inline void setIsRecorded(bool is_recorded) { m_bIsRecorded = is_recorded; }
	inline void setIsRecording(bool is_recording) { m_bIsRecording = is_recording; }
	inline void increaseRecordedFrame() { m_nRecordedFrame++; }
	
signals:
	void wroteSingleFrame(int);
	void finishedBufferAllocation();
	void finishedWritingThread(bool);

private:
	Configuration* m_pConfig;
	QOperationTab* m_pOperationTab;
	QDeviceControlTab* m_pDeviceControlTab;

public:
	bool m_bIsAllocatedWritingBuffer;
	bool m_bIsRecorded;
	bool m_bIsRecording;
	bool m_bIsSaved;
	int m_nRecordedFrame;

public:
	callback2<const char*, bool> SendStatusMessage;
	
public:
    std::vector<float*> m_vectorWritingImageBuffer; // writing buffer
	QString m_fileName;
};

#endif // MEMORYBUFFER_H
