
#include "MemoryBuffer.h"

#include <Doulos/Configuration.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Viewer/QImageView.h>

#include <Common/ImageObject.h>
#include <Common/medfilt.h>

#include <iostream>
#include <deque>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>


MemoryBuffer::MemoryBuffer(QObject *parent) :
    QObject(parent), m_bIsAllocatedWritingBuffer(false),
	m_bIsRecorded(false), m_bIsRecording(false), 
	m_bIsSaved(false), m_nRecordedFrame(0)
{
	m_pOperationTab = (QOperationTab*)parent;
	m_pConfig = m_pOperationTab->getStreamTab()->getMainWnd()->m_pConfiguration;
	m_pDeviceControlTab = m_pOperationTab->getStreamTab()->getDeviceControlTab();
}

MemoryBuffer::~MemoryBuffer()
{
	deallocateWritingBuffer();
}


void MemoryBuffer::allocateWritingBuffer()
{	
	deallocateWritingBuffer();
	{
		int buffer_number = WRITING_IMAGE_SIZE;
		for (int i = 0; i < buffer_number; i++)
		{
			float *writingImageBuffer = new float[8 * m_pConfig->imageSize];
			memset(writingImageBuffer, 0, 8 * m_pConfig->imageSize * sizeof(float));

			m_vectorWritingImageBuffer.push_back(writingImageBuffer);
		}

		char msg[256];
		sprintf(msg, "Writing buffers are successfully allocated. [Image size: %zd Bytes]", 8 * buffer_number * m_pConfig->imageSize * sizeof(float));
		SendStatusMessage(msg, false);
		SendStatusMessage("Now, recording process is available!", false);
	}

	emit finishedBufferAllocation();
}

void MemoryBuffer::deallocateWritingBuffer()
{
	if (m_vectorWritingImageBuffer.size() > 0)
	{
		for (int i = 0; i < m_vectorWritingImageBuffer.size(); i++)
		{
			if (m_vectorWritingImageBuffer.at(i))
			{
				delete[] m_vectorWritingImageBuffer.at(i);
				m_vectorWritingImageBuffer.at(i) = nullptr;
			}
		}
	}
	std::vector<float*> clear_vector;
	clear_vector.swap(m_vectorWritingImageBuffer);

	SendStatusMessage("Writing buffers are successfully disallocated.", false);
}


bool MemoryBuffer::startRecording()
{
	// Check if the previous recorded data is saved
	if (!m_bIsSaved && m_bIsRecorded)
	{
		QMessageBox MsgBox;
		MsgBox.setWindowTitle("Question");
		MsgBox.setIcon(QMessageBox::Question);
		MsgBox.setText("The previous recorded data is not saved.\nWhat would you like to do with this data?");
		MsgBox.setStandardButtons(QMessageBox::Ignore | QMessageBox::Discard | QMessageBox::Cancel);
		MsgBox.setDefaultButton(QMessageBox::Cancel);

		int resp = MsgBox.exec();
		switch (resp)
		{
		case QMessageBox::Ignore:
			break;
		case QMessageBox::Discard:
			m_bIsSaved = true;
			m_bIsRecorded = false;
			return false;
		case QMessageBox::Cancel:
			return false;
		default:
			break;
		}
	}

	// Start Recording
	SendStatusMessage("Data recording is started.", false);
	m_bIsRecorded = false;
		
	// Start Recording
	m_nRecordedFrame = 0;
	m_bIsRecording = true;
	m_bIsSaved = false;

	//// Thread for buffering transfered data (memcpy)
	//std::thread thread_buffering_data = std::thread([&]() {
	//	SendStatusMessage("Data buffering thread is started.", false);
	//	while (1)
	//	{
	//		// Get the buffer from the buffering sync Queue
	//		float* image = m_syncImageBuffering.Queue_sync.pop();
	//		if (image != nullptr)
	//		{
	//			// Body
	//			if (m_nRecordedFrames < WRITING_BUFFER_SIZE)
	//			{
	//				float* buffer = m_queueWritingImageBuffer.front();
	//				m_queueWritingImageBuffer.pop();
	//				memcpy(buffer, image, sizeof(float) * 6 * m_pConfig->imageSize * m_pConfig->imageSize);
	//				m_queueWritingImageBuffer.push(buffer);
	//
	//				m_nRecordedFrames++;
	//				
	//				// Return (push) the buffer to the buffering threading queue
	//				{
	//					std::unique_lock<std::mutex> lock(m_syncImageBuffering.mtx);
	//					m_syncImageBuffering.queue_buffer.push(image);
	//				}
	//			}
	//		}
	//		else
	//			break; ///////////////////////////////////////////////////////////////////////////////////////////////// ?			
	//	}
	//	SendStatusMessage("Data copying thread is finished.", false);
	//});
	//thread_buffering_data.detach();

	return true;
}

void MemoryBuffer::stopRecording()
{
	// Stop recording
	m_bIsRecording = false;
		
	if (m_bIsRecorded) // Not allowed when 'discard'
	{
		// Status update
        uint64_t total_size = (uint64_t)(8 * m_nRecordedFrame * m_pConfig->imageSize * sizeof(float)) / (uint64_t)1024;
		
		char msg[256];
		sprintf(msg, "Data recording is finished normally. \n(Recorded frames: %d frames (%.2f MB)", m_nRecordedFrame, (double)total_size / 1024.0);
		SendStatusMessage(msg, false);
	}
}

bool MemoryBuffer::startSaving()
{
	// Get path to write
	m_fileName = QFileDialog::getSaveFileName(nullptr, "Save As", "", "FLIm raw data (*.data)");
	if (m_fileName == "") return false;
	
	// Start writing thread
	std::thread _thread = std::thread(&MemoryBuffer::write, this);
	_thread.detach();

	return true;
}

//void MemoryBuffer::circulation(int nFramesToCirc)
//{
//	for (int i = 0; i < nFramesToCirc; i++)
//	{
//		uint16_t* buffer = m_queueWritingBuffer.front();
//		m_queueWritingBuffer.pop();
//		m_queueWritingBuffer.push(buffer);
//	}
//}
//
//uint16_t* MemoryBuffer::pop_front()
//{
//	uint16_t* buffer = m_queueWritingBuffer.front();
//	m_queueWritingBuffer.pop();
//
//	return buffer;
//}
//
//void MemoryBuffer::push_back(uint16_t* buffer)
//{
//	m_queueWritingBuffer.push(buffer);
//}


void MemoryBuffer::write()
{	
	qint64 res;
	qint64 samplesToWrite = 8 * m_pConfig->imageSize; /// *m_pConfig->imageSize;

	if (QFile::exists(m_fileName))
	{
        SendStatusMessage("Doulos does not overwrite a recorded data.", false);
		emit finishedWritingThread(true);
		return;
	}

	QString fileTitle, filePath;
	for (int i = 0; i < m_fileName.length(); i++)
	{
		if (m_fileName.at(i) == QChar('.')) fileTitle = m_fileName.left(i);
		if (m_fileName.at(i) == QChar('/')) filePath = m_fileName.left(i);
	}

	// Writing
	QFile file(m_fileName);
	if (file.open(QIODevice::WriteOnly))
	{
		for (int i = 0; i < m_nRecordedFrame; i++)
		{
			// FLIm raw image writing		
			res = file.write(reinterpret_cast<char*>(m_vectorWritingImageBuffer.at(i)), sizeof(float) * samplesToWrite);
			if (!(res == sizeof(float) * samplesToWrite))
			{
				SendStatusMessage("Error occurred while writing...", true);
				emit finishedWritingThread(true);
				return;
			}
		}
		file.close();
		
		QString path = filePath + QString("/scaled_image/");
		QDir().mkpath(path);
		for (int i = 0; i < m_nRecordedFrame; i++)
		{
			// FLIm scaled image writing
			ColorTable temp_ctable;
			IppiSize roi_flim = { m_pConfig->nPixels, m_pConfig->nLines };

			ImageObject imgObjIntensity(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
			ImageObject imgObjLifetime(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));
			ImageObject imgObjTemp(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(ColorTable::gray));
			ImageObject imgObjMerged(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));

			for (int j = 0; j < 4; j++)
			{
				// Intensity image
				float* scanIntensity = m_vectorWritingImageBuffer.at(i) + (0 + j) * roi_flim.width * roi_flim.height;
				ippiScale_32f8u_C1R(scanIntensity, sizeof(float) * roi_flim.width, imgObjIntensity.arr.raw_ptr(), sizeof(uint8_t) * roi_flim.width,
					roi_flim, m_pConfig->flimIntensityRange[j].min, m_pConfig->flimIntensityRange[j].max);
				//(*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObjIntensity.arr.raw_ptr());
				//if (m_nRecordedFrame == 1)
					imgObjIntensity.qindeximg
						.save(path + QString("intensity_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
							.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");
				//else
				//	imgObjIntensity.qindeximg.copy(m_pConfig->galvoFlyingBack, m_pConfig->imageStichingMisSyncPos, 
				//		m_pConfig->imageSize - m_pConfig->galvoFlyingBack, m_pConfig->imageSize - m_pConfig->imageStichingMisSyncPos)
				//	.save(path + QString("intensity_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
				//		.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");

				// Lifetime image
				float* scanLifetime = m_vectorWritingImageBuffer.at(i) + (4 + j) * roi_flim.width * roi_flim.height;
				ippiScale_32f8u_C1R(scanLifetime, sizeof(float) * roi_flim.width, imgObjLifetime.arr.raw_ptr(), sizeof(uint8_t) * roi_flim.width,
					roi_flim, m_pConfig->flimLifetimeRange[j].min, m_pConfig->flimLifetimeRange[j].max);
				//(*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObjLifetime.arr.raw_ptr());
				//if (m_nRecordedFrame == 1)
					imgObjLifetime.qindeximg
						.save(path + QString("lifetime_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
							.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");
				//else
				//	imgObjLifetime.qindeximg.copy(m_pConfig->galvoFlyingBack, m_pConfig->imageStichingMisSyncPos,
				//		m_pConfig->imageSize - m_pConfig->galvoFlyingBack, m_pConfig->imageSize - m_pConfig->imageStichingMisSyncPos)
				//	.save(path + QString("lifetime_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
				//		.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");

				// Merged image
				imgObjLifetime.convertRgb();
				memcpy(imgObjTemp.qindeximg.bits(), imgObjIntensity.arr.raw_ptr(), imgObjTemp.qindeximg.byteCount());
				imgObjTemp.convertRgb();
				ippsMul_8u_Sfs(imgObjLifetime.qrgbimg.bits(), imgObjTemp.qrgbimg.bits(), imgObjMerged.qrgbimg.bits(), imgObjTemp.qrgbimg.byteCount(), 8);
				//if (m_nRecordedFrame == 1)
					imgObjMerged.qrgbimg 
						.save(path + QString("merged_image_ch_%1_avg_%2_i[%3 %4]_l[%5 %6]_%7.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
							.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");
				//else
				//	imgObjMerged.qrgbimg.copy(m_pConfig->galvoFlyingBack, m_pConfig->imageStichingMisSyncPos,
				//		m_pConfig->imageSize - m_pConfig->galvoFlyingBack, m_pConfig->imageSize - m_pConfig->imageStichingMisSyncPos)
				//	.save(path + QString("merged_image_ch_%1_avg_%2_i[%3 %4]_l[%5 %6]_%7.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
				//		.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1)
				//		.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");
			}

			emit wroteSingleFrame(i);
		}			
	}
	else
	{
		SendStatusMessage("Error occurred during writing process.", true);
		return;
	}
	m_bIsSaved = true;

	// Move files
	m_pConfig->setConfigFile("Doulos.ini");
    if (false == QFile::copy("Doulos.ini", fileTitle + ".ini"))
		SendStatusMessage("Error occurred while copying configuration data.", true);
	
	if (false == QFile::copy("Doulos.m", fileTitle + ".m"))
		SendStatusMessage("Error occurred while copying MATLAB processing data.", true);
	
	// Send a signal to notify this thread is finished
	emit finishedWritingThread(false);

	// Status update
	char msg[256];
	sprintf(msg, "Data saving thread is finished normally. (Saved frames: %d frames)", m_nRecordedFrame);
	SendStatusMessage(msg, false);

	QByteArray temp = m_fileName.toLocal8Bit();
	char* filename = temp.data();
	sprintf(msg, "[%s]", filename);
	SendStatusMessage(msg, false);
}
