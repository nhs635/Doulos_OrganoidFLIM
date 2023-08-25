
#include "MemoryBuffer.h"

#include <Doulos/Configuration.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/QVisualizationTab.h>

#include <Doulos/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/QpiProcess/QpiProcess.h>

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
	m_bIsSaved(false), m_nRecordedFrame(0), is_flim(true), dpc_mode(-1), dpc_illum(0)
{
	m_pOperationTab = (QOperationTab*)parent;
	m_pConfig = m_pOperationTab->getStreamTab()->getMainWnd()->m_pConfiguration;
	m_pDeviceControlTab = m_pOperationTab->getStreamTab()->getDeviceControlTab();
}

MemoryBuffer::~MemoryBuffer()
{
	deallocateWritingBuffer();
}


void MemoryBuffer::allocateWritingBuffer(bool _is_flim)
{		
	is_flim = _is_flim;
	deallocateWritingBuffer();
	{
		// Image buffer
		int buffer_number = WRITING_IMAGE_SIZE;
		for (int i = 0; i < buffer_number; i++)
		{
			if (is_flim)
			{
				float *writingImageBuffer = new float[6 * m_pConfig->imageSize];
				memset(writingImageBuffer, 0, 6 * m_pConfig->imageSize * sizeof(float));
				m_vectorWritingImageBuffer.push_back(writingImageBuffer);
			}
			else
			{
				float *writingImageBuffer = new float[4 * CMOS_WIDTH * CMOS_HEIGHT];
				memset(writingImageBuffer, 0, 4 * CMOS_WIDTH * CMOS_HEIGHT * sizeof(float));
				m_vectorWritingImageBuffer.push_back(writingImageBuffer);
			}
		}

		// Pulse buffer (valid only in FLIM acquisition)
		if (is_flim)
		{
			buffer_number = 2 * (m_pConfig->nLines + GALVO_FLYING_BACK + 2);
			for (int i = 0; i < buffer_number; i++)
			{
				uint16_t *writingPulseBuffer = new uint16_t[m_pConfig->nScans * m_pConfig->nTimes];
				memset(writingPulseBuffer, 0, m_pConfig->nScans * m_pConfig->nTimes * sizeof(uint16_t));
				m_vectorWritingPulseBuffer.push_back(writingPulseBuffer);
			}		
		}
		
		// Allocation result
		if (is_flim)
		{
			char msg[256];
			sprintf(msg, "Writing buffers are successfully allocated. [Image size: %zd MBytes]", 6 * WRITING_IMAGE_SIZE * m_pConfig->imageSize * sizeof(float) / 1024 / 1024);
			SendStatusMessage(msg, false);
			sprintf(msg, "Writing buffers are successfully allocated. [Pulse size: %zd MBytes]", m_pConfig->nScans * m_pConfig->nPixels * m_pConfig->nLines * sizeof(uint16_t) / 1024 / 1024);
			SendStatusMessage(msg, false);
			SendStatusMessage("Now, recording process is available!", false);
		}
		else
		{
			char msg[256];
			sprintf(msg, "Writing buffers are successfully allocated. [Image size: %zd Bytes]", 4 * WRITING_IMAGE_SIZE * CMOS_WIDTH * CMOS_HEIGHT * sizeof(float));
			SendStatusMessage(msg, false);
			SendStatusMessage("Now, recording process is available!", false);
		}
	}

	emit finishedBufferAllocation();
}

void MemoryBuffer::deallocateWritingBuffer()
{
	// Clear image buffers
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
	}

	// Clear pulse buffers
	{
		if (m_vectorWritingPulseBuffer.size() > 0)
		{
			for (int i = 0; i < m_vectorWritingPulseBuffer.size(); i++)
			{
				if (m_vectorWritingPulseBuffer.at(i))
				{
					delete[] m_vectorWritingPulseBuffer.at(i);
					m_vectorWritingPulseBuffer.at(i) = nullptr;
				}
			}
		}
		std::vector<uint16_t*> clear_vector;
		clear_vector.swap(m_vectorWritingPulseBuffer);
	}

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

	/// please retrieve this code region when z-stack function is realized.
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
		uint64_t total_size;
		if (is_flim)
			total_size = (uint64_t)(6 * m_nRecordedFrame * m_pConfig->imageSize * sizeof(float)) / (uint64_t)1024;
		else
			total_size = (uint64_t)(4 * m_nRecordedFrame * CMOS_WIDTH * CMOS_HEIGHT * sizeof(float)) / (uint64_t)1024;
		
		char msg[256];
		sprintf(msg, "Data recording is finished normally. \n(Recorded frames: %d frames (%.2f MB)", m_nRecordedFrame, (double)total_size / 1024.0);
		SendStatusMessage(msg, false);
	}
}

bool MemoryBuffer::startSaving()
{
	// Get path to write
	if (is_flim)
		m_fileName = QFileDialog::getSaveFileName(nullptr, "Save As", "", "FLIm raw data (*.data)");
	else
		m_fileName = QFileDialog::getSaveFileName(nullptr, "Save As", "", "DPC raw data (*.dpc)");
	
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
	qint64 samplesToWrite;
	if (is_flim)
		samplesToWrite = 6 * m_pConfig->imageSize; /// *m_pConfig->imageSize;
	else
		samplesToWrite = 4 * CMOS_WIDTH * CMOS_HEIGHT;
	
	if (is_flim)
	{
		if (QFile::exists(m_fileName))
		{
			SendStatusMessage("Doulos does not overwrite a recorded data.", false);
			emit finishedWritingThread(true);
			return;
		}
	}

	QString fileTitle, filePath;
	for (int i = 0; i < m_fileName.length(); i++)
	{
		if (m_fileName.at(i) == QChar('.')) fileTitle = m_fileName.left(i);
		if (m_fileName.at(i) == QChar('/')) filePath = m_fileName.left(i);
	}
	
	// Writing
	QFile file(m_fileName);
	if (file.open(QIODevice::Append)) // QIODevice::WriteOnly))
	{
		// Write image data file
		if (is_flim || (dpc_mode == DPC_PROCESSED))
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
		}
		
		// Write scaled bitmap images
		QString path = filePath + QString("/scaled_image/");
		QDir().mkpath(path);
		for (int i = 0; i < m_nRecordedFrame; i++)
		{
			// FLIm scaled image writing
			if (is_flim)
			{
				ColorTable temp_ctable;
				IppiSize roi_flim = { m_pConfig->nPixels, m_pConfig->nLines };

				ImageObject imgObjIntensity(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
				ImageObject imgObjLifetime(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));
				ImageObject imgObjTemp(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(ColorTable::gray));
				ImageObject imgObjMerged(roi_flim.width, roi_flim.height, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));

				for (int j = 0; j < 3; j++)
				{
					// Intensity image
					float* scanIntensity = m_vectorWritingImageBuffer.at(i) + (0 + j) * roi_flim.width * roi_flim.height;
					ippiScale_32f8u_C1R(scanIntensity, sizeof(float) * roi_flim.width, imgObjIntensity.arr.raw_ptr(), sizeof(uint8_t) * roi_flim.width,
						roi_flim, m_pConfig->flimIntensityRange[j].min, m_pConfig->flimIntensityRange[j].max);
					///(*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObjIntensity.arr.raw_ptr());
					///if (m_nRecordedFrame == 1)
					imgObjIntensity.qindeximg
						.save(path + QString("intensity_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
							.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1).arg(i + 1, 3, 10, (QChar)'0'), "bmp");

					///기존 저장 방법
					///imgObjIntensity.qindeximg
					///	.save(path + QString("intensity_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
					///		.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1).arg(i + 1)); 
					///else
					///	imgObjIntensity.qindeximg.copy(m_pConfig->galvoFlyingBack, m_pConfig->imageStichingMisSyncPos, 
					///		m_pConfig->imageSize - m_pConfig->galvoFlyingBack, m_pConfig->imageSize - m_pConfig->imageStichingMisSyncPos)
					///	.save(path + QString("intensity_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
					///		.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");

					// Lifetime image
					float* scanLifetime = m_vectorWritingImageBuffer.at(i) + (3 + j) * roi_flim.width * roi_flim.height;
					ippiScale_32f8u_C1R(scanLifetime, sizeof(float) * roi_flim.width, imgObjLifetime.arr.raw_ptr(), sizeof(uint8_t) * roi_flim.width,
						roi_flim, m_pConfig->flimLifetimeRange[j].min, m_pConfig->flimLifetimeRange[j].max);
					///(*m_pOperationTab->getStreamTab()->getVisualizationTab()->getMedfilt())(imgObjLifetime.arr.raw_ptr());
					///if (m_nRecordedFrame == 1)
					imgObjLifetime.qindeximg
						.save(path + QString("lifetime_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
							.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1, 3, 10, (QChar)'0'), "bmp");
						
					///imgObjLifetime.qindeximg
					///	.save(path + QString("lifetime_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
					///		.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1)); 
					///else
					///	imgObjLifetime.qindeximg.copy(m_pConfig->galvoFlyingBack, m_pConfig->imageStichingMisSyncPos,
					///		m_pConfig->imageSize - m_pConfig->galvoFlyingBack, m_pConfig->imageSize - m_pConfig->imageStichingMisSyncPos)
					///	.save(path + QString("lifetime_image_ch_%1_avg_%2_[%3 %4]_%5.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
					///		.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");

					// Merged image
					imgObjLifetime.convertRgb();
					memcpy(imgObjTemp.qindeximg.bits(), imgObjIntensity.arr.raw_ptr(), imgObjTemp.qindeximg.byteCount());
					imgObjTemp.convertRgb();
					ippsMul_8u_Sfs(imgObjLifetime.qrgbimg.bits(), imgObjTemp.qrgbimg.bits(), imgObjMerged.qrgbimg.bits(), imgObjTemp.qrgbimg.byteCount(), 8);
					///if (m_nRecordedFrame == 1)
					imgObjMerged.qrgbimg
						.save(path + QString("merged_image_ch_%1_avg_%2_i[%3 %4]_l[%5 %6]_%7.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
							.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1)
							.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1, 3, 10, (QChar)'0'), "bmp");								

					///imgObjMerged.qrgbimg
					///	.save(path + QString("merged_image_ch_%1_avg_%2_i[%3 %4]_l[%5 %6]_%7.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
					///		.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1)
					///		.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1)); 
					///else
					///	imgObjMerged.qrgbimg.copy(m_pConfig->galvoFlyingBack, m_pConfig->imageStichingMisSyncPos,
					///		m_pConfig->imageSize - m_pConfig->galvoFlyingBack, m_pConfig->imageSize - m_pConfig->imageStichingMisSyncPos)
					///	.save(path + QString("merged_image_ch_%1_avg_%2_i[%3 %4]_l[%5 %6]_%7.bmp").arg(j + 1).arg(m_pConfig->imageAveragingFrames)
					///		.arg(m_pConfig->flimIntensityRange[j].min, 2, 'f', 1).arg(m_pConfig->flimIntensityRange[j].max, 2, 'f', 1)
					///		.arg(m_pConfig->flimLifetimeRange[j].min, 2, 'f', 1).arg(m_pConfig->flimLifetimeRange[j].max, 2, 'f', 1).arg(i + 1), "bmp");
				}
			}
			else
			{
				ColorTable temp_ctable;
				IppiSize roi_dpc = { CMOS_WIDTH, CMOS_HEIGHT };
				
				if (dpc_mode == DPC_LIVE)
				{
					ImageObject imgObjLive(roi_dpc.width, roi_dpc.height, temp_ctable.m_colorTableVector.at(ColorTable::gray));

					// Live image
					QString image_type;
					switch (dpc_illum)
					{
					case 0:
						image_type = "off";
						break;
					case 1:
						image_type = "brightfield";
						break;
					case 2:
						image_type = "darkfield";
						break;
					case 3:
						image_type = "top";
						break;
					case 4:
						image_type = "bottom";
						break;
					case 5:
						image_type = "left";
						break;
					case 6:
						image_type = "right";
						break;
					}

					float *scanLive = m_vectorWritingImageBuffer.at(i);
					ippiScale_32f8u_C1R(scanLive, sizeof(float) * roi_dpc.width, imgObjLive.arr.raw_ptr(), sizeof(uint8_t) * roi_dpc.width,
						roi_dpc, m_pConfig->liveIntensityRange.min, m_pConfig->liveIntensityRange.max);
					imgObjLive.qindeximg
						.save(path + QString("%1_image_[%2 %3]_%4.bmp")
							.arg(image_type).arg(m_pConfig->liveIntensityRange.min).arg(m_pConfig->liveIntensityRange.max).arg(i + 1, 3, 10, (QChar)'0'), "bmp");
				}
				else if (dpc_mode == DPC_PROCESSED)
				{
					// Get QPI objects
					QpiProcess* pQpi = m_pOperationTab->getDataAcq()->getQpi();
					// 0 top 1 left 2 bottom 3 right

					std::vector<np::FloatArray2> vecIllumImages;
					for (int j = 0; j < 4; j++)
					{
						np::FloatArray2 illum(roi_dpc.width, roi_dpc.height);
						memcpy(illum, m_vectorWritingImageBuffer.at(i) + j * roi_dpc.width * roi_dpc.height, sizeof(float) * roi_dpc.width * roi_dpc.height);
						vecIllumImages.push_back(illum);
					}

					// Brightfield image
					pQpi->getBrightfield(vecIllumImages);

					// DPC top-bottom
					pQpi->getDpc(vecIllumImages, top_bottom);

					// DPC left-right
					pQpi->getDpc(vecIllumImages, left_right);

					// DPC fusion
					pQpi->getQpi(vecIllumImages);

					// Image objects
					ImageObject imgObjBrightField(roi_dpc.width, roi_dpc.height, temp_ctable.m_colorTableVector.at(ColorTable::gray));
					ImageObject imgObjDpcTb(roi_dpc.width, roi_dpc.height, temp_ctable.m_colorTableVector.at(m_pConfig->dpcColorTable));
					ImageObject imgObjDpcLr(roi_dpc.width, roi_dpc.height, temp_ctable.m_colorTableVector.at(m_pConfig->dpcColorTable));
					ImageObject imgObjPhase(roi_dpc.width, roi_dpc.height, temp_ctable.m_colorTableVector.at(m_pConfig->phaseColorTable));

					// Brightfield image
					float* scanBrightfield = pQpi->brightfield.raw_ptr();
					ippiScale_32f8u_C1R(scanBrightfield, sizeof(float) * roi_dpc.width, imgObjBrightField.arr.raw_ptr(), sizeof(uint8_t) * roi_dpc.width,
						roi_dpc, m_pConfig->liveIntensityRange.min, m_pConfig->liveIntensityRange.max);
					imgObjBrightField.qindeximg
						.save(path + QString("brightfield_image_[%1 %2]_%3.bmp")
							.arg(m_pConfig->liveIntensityRange.min).arg(m_pConfig->liveIntensityRange.max).arg(i + 1, 3, 10, (QChar)'0'), "bmp");

					// DPC image
					float* scanDpcTb = pQpi->dpc_tb.raw_ptr();
					ippiScale_32f8u_C1R(scanDpcTb, sizeof(float) * roi_dpc.width, imgObjDpcTb.arr.raw_ptr(), sizeof(uint8_t) * roi_dpc.width,
						roi_dpc, m_pConfig->dpcRange.min, m_pConfig->dpcRange.max);
					imgObjDpcTb.qindeximg
						.save(path + QString("dpc_tb_image_[%1 %2]_%3.bmp")
							.arg(m_pConfig->dpcRange.min, 3, 'f', 2).arg(m_pConfig->dpcRange.max, 3, 'f', 2).arg(i + 1, 3, 10, (QChar)'0'), "bmp");

					float* scanDpcLr = pQpi->dpc_lr.raw_ptr();
					ippiScale_32f8u_C1R(scanDpcLr, sizeof(float) * roi_dpc.width, imgObjDpcLr.arr.raw_ptr(), sizeof(uint8_t) * roi_dpc.width,
						roi_dpc, m_pConfig->dpcRange.min, m_pConfig->dpcRange.max);
					imgObjDpcLr.qindeximg
						.save(path + QString("dpc_lr_image_[%1 %2]_%3.bmp")
							.arg(m_pConfig->dpcRange.min, 3, 'f', 2).arg(m_pConfig->dpcRange.max, 3, 'f', 2).arg(i + 1, 3, 10, (QChar)'0'), "bmp");

					// Phase image
					float* scanPhase = pQpi->phase.raw_ptr();
					ippiScale_32f8u_C1R(scanPhase, sizeof(float) * roi_dpc.width, imgObjPhase.arr.raw_ptr(), sizeof(uint8_t) * roi_dpc.width,
						roi_dpc, m_pConfig->phaseRange.min, m_pConfig->phaseRange.max);
					imgObjPhase.qindeximg
						.save(path + QString("phase_image_[%1 %2]_%3.bmp")
							.arg(m_pConfig->phaseRange.min, 3, 'f', 2).arg(m_pConfig->phaseRange.max, 3, 'f', 2).arg(i + 1, 3, 10, (QChar)'0'), "bmp");

					// Phase raw data
					QFile phase_file(fileTitle + ".phase");
					if (phase_file.open(QIODevice::Append)) // QIODevice::))
					{
						res = phase_file.write(reinterpret_cast<char*>(pQpi->phase.raw_ptr()), sizeof(float) * pQpi->phase.length());
						if (!(res == sizeof(float) * pQpi->phase.length()))
						{
							SendStatusMessage("Error occurred while writing...", true);
							emit finishedWritingThread(true);
							return;
						}
						phase_file.close();
					}
				}
			}

			emit wroteSingleFrame(i);
		}			

		// Write raw pulse data file		
		if (is_flim)
		{
			if (m_vectorWritingPulseBuffer.at(0)[0] != 0)
			{
				FLImProcess *pFLIm = m_pOperationTab->getDataAcq()->getFLIm();

				QFile file_pulse(fileTitle + ".pulse");
				if (file_pulse.open(QIODevice::WriteOnly))
				{
					for (int i = 0; i < m_vectorWritingPulseBuffer.size(); i++)
					{
						// FLIm raw pulse writing		
						Uint16Array2 pulse_buffer(m_vectorWritingPulseBuffer.at(i), m_pConfig->nScans, m_pConfig->nTimes);

						int roi_width = pFLIm->_params.ch_start_ind[4] - pFLIm->_params.ch_start_ind[0] + 2;
						Uint16Array2 pulse_roi_buffer(roi_width, m_pConfig->nTimes);
						memset(pulse_roi_buffer, 0, sizeof(uint16_t) * pulse_roi_buffer.length());

						ippiCopy_16u_C1R(&pulse_buffer(pFLIm->_params.ch_start_ind[0], 0), sizeof(uint16_t) * pulse_buffer.size(0),
							pulse_roi_buffer, sizeof(uint16_t) * pulse_roi_buffer.size(0), { roi_width - 2, m_pConfig->nTimes });

						// Find auto background value
						double bg_auto;
						int offset = pFLIm->_params.ch_start_ind[0];
						FloatArray2 bg_region(m_pConfig->nScans - offset - roi_width - 2, m_pConfig->nTimes);
						ippiConvert_16u32f_C1R(&pulse_buffer(offset + roi_width - 2, 0), sizeof(uint16_t) * m_pConfig->nScans,
							&bg_region(0, 0), sizeof(float) * bg_region.size(0), { bg_region.size(0), bg_region.size(1) });
						ippiMean_32f_C1R(&bg_region(0, 0), sizeof(float) * bg_region.size(0), { bg_region.size(0), bg_region.size(1) }, &bg_auto, ippAlgHintFast);

						float bg_auto1 = (float)bg_auto;
						memcpy(&pulse_roi_buffer(roi_width - 2, 0), (uint16_t*)&bg_auto1, sizeof(float));
						
						// write
						res = file_pulse.write(reinterpret_cast<char*>(pulse_roi_buffer.raw_ptr()), sizeof(uint16_t) * pulse_roi_buffer.length());
						if (!(res == sizeof(uint16_t) * pulse_roi_buffer.length()))
						{
							SendStatusMessage("Error occurred while writing...", true);
							emit finishedWritingThread(true);
							return;
						}
					}
					file_pulse.close();
				}
			}
		}
	}
	else
	{
		SendStatusMessage("Error occurred during writing process.", true);
		return;
	}
	m_bIsSaved = true;

	// Move files
	if (is_flim)
	{
		m_pConfig->setConfigFile("Doulos.ini");
		if (false == QFile::copy("Doulos.ini", fileTitle + ".ini"))
			SendStatusMessage("Error occurred while copying configuration data.", true);
	}
	else
	{
		m_pConfig->setConfigFile("Doulos.ini");
		if (false == QFile::copy("Doulos.ini", fileTitle + "_dpc.ini"))
			SendStatusMessage("Error occurred while copying configuration data.", true);
	}

	if (false == QFile::copy("Doulos.m", fileTitle + ".m"))
		SendStatusMessage("Error occurred while copying MATLAB processing data.", true);

	// Send a signal to notify this thread is finished
	emit finishedWritingThread(false);

	// Open saved folder
	QDesktopServices::openUrl(QUrl("file:///" + filePath));

	// Status update
	char msg[256];
	sprintf(msg, "Data saving thread is finished normally. (Saved frames: %d frames)", m_nRecordedFrame);
	SendStatusMessage(msg, false);

	QByteArray temp = m_fileName.toLocal8Bit();
	char* filename = temp.data();
	sprintf(msg, "[%s]", filename);
	SendStatusMessage(msg, false);
}
