
#include "QOperationTab.h"

#include <Doulos/Configuration.h>
#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QDeviceControlTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/ThreadManager.h>
#include <MemoryBuffer/MemoryBuffer.h>

#include <iostream>
#include <thread>
#include <chrono>


QOperationTab::QOperationTab(QWidget *parent) :
    QDialog(parent)
{
	// Set main window objects
    m_pStreamTab = dynamic_cast<QStreamTab*>(parent);
    m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;

	// Create data acquisition object
    m_pDataAcquisition = new DataAcquisition(m_pConfig);

	// Create memory buffer objectv
    m_pMemoryBuffer = new MemoryBuffer(this);
	m_pMemoryBuffer->SendStatusMessage += [&](const char* msg, bool is_error) {
		if (is_error) setRecordingButton(false);
        QString qmsg = QString::fromUtf8(msg);
		emit m_pStreamTab->sendStatusMessage(qmsg, is_error);
	};

    // Create widgets for acquisition / recording / saving operation
    m_pToggleButton_Acquisition = new QPushButton(this);
    m_pToggleButton_Acquisition->setCheckable(true);
    m_pToggleButton_Acquisition->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_pToggleButton_Acquisition->setFixedHeight(30);
    m_pToggleButton_Acquisition->setText("Start &Acquisition");

    m_pToggleButton_Recording = new QPushButton(this);
    m_pToggleButton_Recording->setCheckable(true);
    m_pToggleButton_Recording->setFixedHeight(30);
    m_pToggleButton_Recording->setText("Start &Recording");
	m_pToggleButton_Recording->setDisabled(true);

    m_pToggleButton_Saving = new QPushButton(this);
    m_pToggleButton_Saving->setCheckable(true);
    m_pToggleButton_Saving->setFixedHeight(30);
    m_pToggleButton_Saving->setText("&Save Recorded Data");
	m_pToggleButton_Saving->setDisabled(true);

    // Create a progress bar (general purpose?)
    m_pProgressBar = new QProgressBar(this);
    m_pProgressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pProgressBar->setFormat("");
    m_pProgressBar->setValue(0);

    // Set layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(2);
	
    QHBoxLayout *pHBoxLayout = new QHBoxLayout;
    pHBoxLayout->setSpacing(2);

    pHBoxLayout->addWidget(m_pToggleButton_Acquisition);
    pHBoxLayout->addWidget(m_pToggleButton_Recording);
    pHBoxLayout->addWidget(m_pToggleButton_Saving);

    m_pVBoxLayout->addItem(pHBoxLayout);
    m_pVBoxLayout->addWidget(m_pProgressBar);
    m_pVBoxLayout->addStretch(1);

    setLayout(m_pVBoxLayout);


	// Connect signal and slot
    connect(m_pToggleButton_Acquisition, SIGNAL(toggled(bool)), this, SLOT(operateDataAcquisition(bool)));
	connect(m_pToggleButton_Recording, SIGNAL(toggled(bool)), this, SLOT(operateDataRecording(bool)));
    connect(m_pToggleButton_Saving, SIGNAL(toggled(bool)), this, SLOT(operateDataSaving(bool)));
    connect(m_pMemoryBuffer, SIGNAL(finishedBufferAllocation()), this, SLOT(setAcqRecEnable()));
	connect(m_pMemoryBuffer, SIGNAL(finishedWritingThread(bool)), this, SLOT(setSaveButtonDefault(bool)));
	connect(m_pMemoryBuffer, SIGNAL(wroteSingleFrame(int)), m_pProgressBar, SLOT(setValue(int)));
}

QOperationTab::~QOperationTab()
{
    if (m_pDataAcquisition) delete m_pDataAcquisition;
    if (m_pMemoryBuffer) delete m_pMemoryBuffer;
}


void QOperationTab::changedTab(bool change)
{	
//	if (change)
//		m_pToggleButton_Acquisition->setChecked(!change);

//	m_pToggleButton_Acquisition->setDisabled(change);
//	m_pPushButton_DigitizerSetup->setDisabled(change);

	(void)change;
}

void QOperationTab::operateDataAcquisition(bool toggled)
{
    if (toggled) // Start Data Acquisition
    {
        if (m_pDataAcquisition->InitializeAcquistion())
        {
            // Start Thread Process
            m_pStreamTab->m_pThreadVisualization->startThreading();
            m_pStreamTab->m_pThreadFlimProcess->startThreading();			

            // Start Data Acquisition
            if (m_pDataAcquisition->StartAcquisition())
            {
                m_pToggleButton_Acquisition->setText("Stop &Acquisition");

                ///m_pToggleButton_Acquisition->setDisabled(true);
                m_pToggleButton_Recording->setEnabled(true);

                std::thread allocate_writing_buffer([&]() {
                    m_pMemoryBuffer->allocateWritingBuffer();
                });
                allocate_writing_buffer.detach();
            }
			else
			{
				m_pToggleButton_Acquisition->setChecked(false); // When start acquisition is failed...
				return;
			}
			
			// Set image size widgets
			m_pStreamTab->setYLinesWidgets(false);

			// Set stitching mode
			if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
			{
				m_pStreamTab->getXStepLabel()->setDisabled(true);
				m_pStreamTab->getYStepLabel()->setDisabled(true);
				m_pStreamTab->getXStepLineEdit()->setDisabled(true);
				m_pStreamTab->getYStepLineEdit()->setDisabled(true);
			}
			
			// Set Alazar Control widgets
			m_pStreamTab->getDeviceControlTab()->getAlazarDigitizerControl()->setChecked(true);

			// Start Resonant & Galvo-scan & Apply FLIm triggers & PMT gain voltage
			m_pStreamTab->getDeviceControlTab()->getResonantScanControl()->setChecked(true);
			m_pStreamTab->getDeviceControlTab()->getResonantScanVoltageControl()->setChecked(true);
			m_pStreamTab->getDeviceControlTab()->getResonantScanVoltageSpinBox()->setValue(m_pConfig->resonantScanVoltage);
			m_pStreamTab->getDeviceControlTab()->getGalvoScanControl()->setChecked(true);
			m_pStreamTab->getDeviceControlTab()->getFlimLaserControl()->setChecked(true);
			m_pStreamTab->getDeviceControlTab()->getPmtGainControl()->setChecked(true);						
			std::this_thread::sleep_for(std::chrono::milliseconds(500));

			// Start laser triggering
			m_pStreamTab->getDeviceControlTab()->getFlimLaserTrigControl()->setChecked(true);
        }
		else
		{
			m_pToggleButton_Acquisition->setChecked(false); // When initialization is failed...
			return;
		}
    }
    else // Stop Data Acquisition
    {
        // Stop Thread Process
        m_pDataAcquisition->StopAcquisition();
        m_pStreamTab->m_pThreadFlimProcess->stopThreading();
        m_pStreamTab->m_pThreadVisualization->stopThreading();

		std::thread deallocate_writing_buffer([&]() {
			m_pMemoryBuffer->deallocateWritingBuffer();
		});
		deallocate_writing_buffer.detach();

        m_pToggleButton_Acquisition->setText("Start &Acquisition");
        m_pToggleButton_Recording->setDisabled(true);
		
		// Stop laser triggering
		m_pStreamTab->getDeviceControlTab()->getFlimLaserTrigControl()->setChecked(false);

		// Stop Resonant & Galvo-scan & Disapply PMT gain voltage
		m_pStreamTab->getDeviceControlTab()->getResonantScanControl()->setChecked(false);
		m_pStreamTab->getDeviceControlTab()->getResonantScanVoltageControl()->setChecked(false);
		m_pStreamTab->getDeviceControlTab()->getGalvoScanControl()->setChecked(false);
		m_pStreamTab->getDeviceControlTab()->getFlimLaserControl()->setChecked(false);		
		m_pStreamTab->getDeviceControlTab()->getPmtGainControl()->setChecked(false);
				
		// Set Alazar Control widgets
		m_pStreamTab->getDeviceControlTab()->getAlazarDigitizerControl()->setChecked(false);

		// Set stitching mode
		if (m_pStreamTab->getImageStitchingCheckBox()->isChecked())
		{
			m_pStreamTab->getXStepLabel()->setEnabled(true);
			m_pStreamTab->getYStepLabel()->setEnabled(true);
			m_pStreamTab->getXStepLineEdit()->setEnabled(true);
			m_pStreamTab->getYStepLineEdit()->setEnabled(true);
		}

		// Set image size widgets
		if (!m_pStreamTab->getDeviceControlTab()->getGalvoScanControl()->isChecked())
			m_pStreamTab->setYLinesWidgets(true);
    }
}

void QOperationTab::operateDataRecording(bool toggled)
{
    if (toggled) // Start Data Recording
    {
		if (m_pMemoryBuffer->startRecording())
		{
			m_pToggleButton_Recording->setText("Stop &Recording");
			m_pToggleButton_Acquisition->setDisabled(true);
			m_pToggleButton_Saving->setDisabled(true);
			m_pStreamTab->setAveragingWidgets(false);
		}
		else
		{
			m_pToggleButton_Recording->setChecked(false);

			if (m_pMemoryBuffer->m_bIsSaved) // When select "discard"
				m_pToggleButton_Saving->setDisabled(true);
		}
    }
    else // Stop DataRecording
    {
		m_pMemoryBuffer->stopRecording();

		m_pToggleButton_Recording->setText("Start &Recording");
		m_pToggleButton_Acquisition->setEnabled(true);
		m_pToggleButton_Saving->setEnabled(m_pMemoryBuffer->m_bIsRecorded);
		m_pStreamTab->setAveragingWidgets(true);

		if (m_pMemoryBuffer->m_nRecordedFrame > 1)
			m_pProgressBar->setRange(0, m_pMemoryBuffer->m_nRecordedFrame - 1);
		else
			m_pProgressBar->setRange(0, 1);
		m_pProgressBar->setValue(0);  
    }
}

void QOperationTab::operateDataSaving(bool toggled)
{
    if (toggled)
    {
		printf("m_pMemoryBuffer: %p\n", m_pMemoryBuffer);
		if (m_pMemoryBuffer)
		{
			if (m_pMemoryBuffer->startSaving())
			{
				m_pToggleButton_Saving->setText("Saving...");
				m_pToggleButton_Recording->setDisabled(true);
				m_pToggleButton_Saving->setDisabled(true);
				m_pProgressBar->setFormat("Writing recorded data... %p%");
			}
			else
				m_pToggleButton_Saving->setChecked(false);
		}
    }
}


void QOperationTab::setAcqRecEnable()
{
	m_pToggleButton_Acquisition->setEnabled(true); 
	m_pToggleButton_Recording->setEnabled(true);
}

void QOperationTab::setSaveButtonDefault(bool error)
{
	m_pProgressBar->setFormat("");
	m_pToggleButton_Saving->setText("&Save Recorded Data");
	m_pToggleButton_Saving->setChecked(false);
	m_pToggleButton_Saving->setDisabled(!error);
	if (m_pToggleButton_Acquisition->isChecked())
		m_pToggleButton_Recording->setEnabled(true);
}
