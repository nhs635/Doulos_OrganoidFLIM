#-------------------------------------------------
#
# Project created by QtCreator 2018-08-08T16:07:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport

TARGET = Doulos
TEMPLATE = app

CONFIG += console
RC_FILE += Doulos.rc

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



win32 {
    INCLUDEPATH += $$PWD/include

    LIBS += $$PWD/lib/ATSApi.lib
    LIBS += $$PWD/lib/NIDAQmx.lib
    LIBS += $$PWD/lib/intel64_win/ippcore.lib \
            $$PWD/lib/intel64_win/ippi.lib \
            $$PWD/lib/intel64_win/ipps.lib
    debug {
        LIBS += $$PWD/lib/intel64_win/vc14/tbb_debug.lib \
                $$PWD/lib/tis_udshl12d_x64.lib
    }
    release {
        LIBS += $$PWD/lib/intel64_win/vc14/tbb.lib \
                $$PWD/lib/tis_udshl12_x64.lib
    }
    LIBS += $$PWD/lib/intel64_win/mkl_core.lib \
            $$PWD/lib/intel64_win/mkl_tbb_thread.lib \
            $$PWD/lib/intel64_win/mkl_intel_lp64.lib
}


SOURCES += Doulos/Doulos.cpp \
    Doulos/MainWindow.cpp \
    Doulos/QStreamTab.cpp \
    Doulos/QOperationTab.cpp \
    Doulos/QDeviceControlTab.cpp \
    Doulos/QVisualizationTab.cpp \
    Doulos/Viewer/QScope.cpp \
    Doulos/Viewer/QImageView.cpp \
    Doulos/Dialog/FlimCalibDlg.cpp

SOURCES += DataAcquisition/AlazarDAQ/AlazarDAQ.cpp \
    DataAcquisition/FLImProcess/FLImProcess.cpp \
    DataAcquisition/QpiProcess/QpiProcess.cpp \
    DataAcquisition/ImagingSource/ImagingSource.cpp \
    DataAcquisition/ThreadManager.cpp \
    DataAcquisition/DataAcquisition.cpp

SOURCES += MemoryBuffer/MemoryBuffer.cpp

SOURCES += DeviceControl/FLImControl/PmtGainControl.cpp \
    DeviceControl/FLImControl/FLImTrigger.cpp \
    DeviceControl/IPGPhotonicsLaser/DigitalInput/DigitalInput.cpp \
    DeviceControl/IPGPhotonicsLaser/DigitalOutput/DigitalOutput.cpp \
    DeviceControl/IPGPhotonicsLaser/IPGPhotonicsLaser.cpp \
    DeviceControl/ResonantScan/PulseTrainGenerator.cpp \
    DeviceControl/ResonantScan/ResonantScan.cpp \
    DeviceControl/GalvoScan/GalvoScan.cpp \
    DeviceControl/GalvoScan/TwoEdgeTriggerEnable.cpp \
    DeviceControl/NanoscopeStage/NanoscopeStage.cpp \
    DeviceControl/DpcIllumination/DpcIllumination.cpp


HEADERS += Doulos/Configuration.h \
    Doulos/MainWindow.h \
    Doulos/QStreamTab.h \
    Doulos/QOperationTab.h \
    Doulos/QDeviceControlTab.h \
    Doulos/QVisualizationTab.h \
    Doulos/Viewer/QScope.h \
    Doulos/Viewer/QImageView.h \
    Doulos/Dialog/FlimCalibDlg.h

HEADERS += DataAcquisition/AlazarDAQ/AlazarDAQ.h \
    DataAcquisition/FLImProcess/FLImProcess.h \
    DataAcquisition/QpiProcess/QpiProcess.h \
    DataAcquisition/ImagingSource/ImagingSource.h \
    DataAcquisition/ThreadManager.h \
    DataAcquisition/DataAcquisition.h

HEADERS += MemoryBuffer/MemoryBuffer.h

HEADERS += DeviceControl/FLImControl/PmtGainControl.h \
    DeviceControl/FLImControl/FLImTrigger.h \
    DeviceControl/IPGPhotonicsLaser/DigitalInput/DigitalInput.h \
    DeviceControl/IPGPhotonicsLaser/DigitalOutput/DigitalOutput.h \
    DeviceControl/IPGPhotonicsLaser/IPGPhotonicsLaser.h \
    DeviceControl/ResonantScan/PulseTrainGenerator.h \
    DeviceControl/ResonantScan/ResonantScan.h \
    DeviceControl/GalvoScan/GalvoScan.h \
    DeviceControl/GalvoScan/TwoEdgeTriggerEnable.h \
    DeviceControl/NanoscopeStage/NanoscopeStage.h \
    DeviceControl/DpcIllumination/DpcIllumination.h \
    DeviceControl/QSerialComm.h


FORMS   += Doulos/MainWindow.ui
