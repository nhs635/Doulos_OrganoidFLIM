#ifndef _QSERIAL_COMM_H_
#define _QSERIAL_COMM_H_

#include <QObject>
#include <QtSerialPort/QSerialPort>

#include <iostream>
#include <thread>

#include <Common/callback.h>


class QSerialComm : public QObject
{
	Q_OBJECT

public:
	QSerialComm(QObject *parent = 0) :
		QObject(parent), m_pSerialPort(nullptr), m_bIsConnected(false)
	{
		m_pSerialPort = new QSerialPort;
		connect(m_pSerialPort, SIGNAL(readyRead()), this, SLOT(readSerialPort()));
	}

	~QSerialComm()
	{
		closeSerialPort();
		if (m_pSerialPort) delete m_pSerialPort;
	}

public:
	inline const char* getPortName() { return m_PortName; }
	inline void setPortName(const char* port_name) { m_PortName = port_name; }
	inline bool getConnectState() { return m_bIsConnected; }

public:
	bool openSerialPort(QString portName, 
		QSerialPort::BaudRate baudRate = QSerialPort::Baud9600, 
		QSerialPort::DataBits dataBits = QSerialPort::Data8,
		QSerialPort::Parity parity = QSerialPort::NoParity, 
		QSerialPort::StopBits stopBits = QSerialPort::OneStop, 
		QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl)
	{
		m_pSerialPort->setPortName(portName);
		m_pSerialPort->setBaudRate(baudRate);
		m_pSerialPort->setDataBits(dataBits);
		m_pSerialPort->setParity(parity);
		m_pSerialPort->setStopBits(stopBits);
		m_pSerialPort->setFlowControl(flowControl);

		if (!m_pSerialPort->open(QIODevice::ReadWrite))
		{
			closeSerialPort();
			return false;
		}

		m_bIsConnected = true;
		
		return true;
	}

	void closeSerialPort()
	{
		if (m_pSerialPort->isOpen())
			m_pSerialPort->close();
		m_bIsConnected = false;
	}

	bool writeSerialPort(char* data, qint64 len = 0)
	{
		if (m_pSerialPort->isOpen())
		{
			qint64 nWrote;
			if (len == 0)
				nWrote = m_pSerialPort->write(data);
			else
				nWrote = m_pSerialPort->write(data, len);

			if (nWrote != 0)
				return true;
		}
		return false;
	}

	void waitUntilResponse(int msec = 100)
	{
		m_pSerialPort->waitForReadyRead(msec);
	}
	
public slots:
	void readSerialPort()
	{
		qint64 nRead;
		char buffer[50];

		nRead = m_pSerialPort->read(buffer, 50);
		DidReadBuffer(buffer, nRead);
	}

public:
	callback2<char*, qint64> DidReadBuffer;
	
private:
	QSerialPort* m_pSerialPort;
	const char* m_PortName;
	bool m_bIsConnected;
};

#endif // _SERIAL_COMM_H_