#ifndef QDEVICECONTROLTAB_H
#define QDEVICECONTROLTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>

#include <iostream>
#include <mutex>
#include <condition_variable>


class QStreamTab;

class PmtGainControl;
class FlimTrigger;
class IPGPhotonicsLaser;
class FlimCalibDlg;

class PulseTrainGenerator;
class ResonantScan;
class GalvoScan;
#if (CRS_DIR_FACTOR == 2)
class TwoEdgeTriggerEnable;
#endif
class NanoscopeStage;
class DpcIllumination;

class QImageView;


class QDeviceControlTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QDeviceControlTab(QWidget *parent = nullptr);
    virtual ~QDeviceControlTab();

// Methods //////////////////////////////////////////////
protected:
	void closeEvent(QCloseEvent*);

public:
	void setControlsStatus(bool enabled);

public: ////////////////////////////////////////////////////////////////////////////////////////////////
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
    inline QStreamTab* getStreamTab() const { return m_pStreamTab; }

	inline QGroupBox* getFlimControlGroupBox() const { return m_pGroupBox_FlimControl; }
	inline QGroupBox* getScannerControlGroupBox() const { return m_pGroupBox_ScannerControl; }
	inline QGroupBox* getNanoscopeStageControlGroupBox() const { return m_pGroupBox_NanoscopeStageControl; }
	inline QGroupBox* getDpcIlluminationGroupBox() const { return m_pGroupBox_DpcIlluminationControl; }

	inline QCheckBox* getPmtGainControl() const { return m_pCheckBox_PmtGainControl; }
	inline QCheckBox* getFlimLaserTrigControl() const { return m_pCheckBox_FlimLaserTrigControl; }
	inline QCheckBox* getFlimLaserControl() const { return m_pCheckBox_FlimLaserControl; }

    inline FlimCalibDlg* getFlimCalibDlg() const { return m_pFlimCalibDlg; }
    inline QCheckBox* getAlazarDigitizerControl() const { return m_pCheckBox_AlazarDigitizerControl; }	

    inline QCheckBox* getGalvoScanControl() const { return m_pCheckBox_GalvoScanControl; }
	inline GalvoScan* getGalvoScan() const { return m_pGalvoScan; }

	inline QCheckBox* getNanoscopeStageControl() const { return m_pCheckBox_NanoscopeStageControl; }
	inline NanoscopeStage* getNanoscopeStage() const { return m_pNanoscopeStage; }	

	inline void connectDpcIllumination(bool enabled) { connectDpcIlluminationControl(enabled); }
	inline void setDpcIllumPattern(int pattern) { changeIlluminationMode(pattern); }
	inline int getDpcIllumPattern() { return m_pButtonGroup_Illumination->checkedId(); }
	inline QRadioButton* getRadioButtonOff() { return m_pRadioButton_Off; }
	inline QRadioButton* getRadioButtonBrightField() { return m_pRadioButton_BrightField; }
	inline QRadioButton* getRadioButtonTop() { return m_pRadioButton_Top; }
	inline QRadioButton* getRadioButtonBottom() { return m_pRadioButton_Bottom; }
	inline QRadioButton* getRadioButtonLeft() { return m_pRadioButton_Left; }
	inline QRadioButton* getRadioButtonRight() { return m_pRadioButton_Right; }
	inline DpcIllumination* getDpcIlluminationControl() const { return m_pDpcIllumControl; }
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    void createPmtGainControl();
    void createFlimTriggeringControl();
	void createFlimLaserControl();
    void createFlimCalibControl();
    void createScanControl();
	void createNanoscopeStageControl();
	void createDpcIlluminationControl();

private slots: /////////////////////////////////////////////////////////////////////////////////////////
    // FLIm PMT Gain Control
    void applyPmtGainVoltage(bool);
    void changePmtGainVoltage(const QString &);

    // FLIm Laser Trigger Control
    void startFlimTriggering(bool);
    void changeFlimLaserRepRate(const QString &);

    // FLIm Laser Control
    void connectFlimLaser(bool);
	void adjustLaserPower(int);

    // FLIm Calibration Control
    void connectAlazarDigitizer(bool);
    void createFlimCalibDlg();
    void deleteFlimCalibDlg();

    // Resonant & Galvano Scanner Control	
    void connectGalvanoMirror(bool);
    void changeGalvoScanVoltage(const QString &);
    void changeGalvoScanVoltageOffset(const QString &);

	// Nanoscope Stage Control
	void connectNanoscopeStage(bool);
	void stageScan(int, int);
	void getCurrentPosition();

	// DPC Illumination Control
	void connectDpcIlluminationControl(bool);
	void changeIlluminationMode(int);

signals: ////////////////////////////////////////////////////////////////////////////////////////////////
	void startStageScan(int, int);
	void monitoring();

private: ////////////////////////////////////////////////////////////////////////////////////////////////
    // PMT Gain Control & FLIm Laser Triggering
    PmtGainControl* m_pPmtGainControl;	
    FlimTrigger* m_pFlimTrigControlLaser; 
	FlimTrigger* m_pFlimTrigControlDAQ;

	// FLIm pulsed Laser Control
	IPGPhotonicsLaser* m_pIPGPhotonicsLaser;

    // Galvo Scanner Control	
    GalvoScan* m_pGalvoScan;	
#if (CRS_DIR_FACTOR == 2)
	TwoEdgeTriggerEnable* m_pTwoEdgeTriggerEnable;
#endif

	// Nanoscope Stage Control
	NanoscopeStage* m_pNanoscopeStage;	

	// DPC Illumination Control
	DpcIllumination* m_pDpcIllumControl;
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

    // Layout
    QVBoxLayout *m_pVBoxLayout;

	// FLIM Layout
	QVBoxLayout *m_pVBoxLayout_FlimControl;
    QGroupBox *m_pGroupBox_FlimControl;
	QGroupBox *m_pGroupBox_ScannerControl;
	QGroupBox *m_pGroupBox_NanoscopeStageControl;
	QGroupBox *m_pGroupBox_DpcIlluminationControl;

    // Widgets for FLIm control	// Gain control
    QCheckBox *m_pCheckBox_PmtGainControl;
    QLineEdit *m_pLineEdit_PmtGainVoltage;
    QLabel *m_pLabel_PmtGainVoltage;

    // Widgets for FLIm control // Laser trig control
    QCheckBox *m_pCheckBox_FlimLaserTrigControl;
    QLabel *m_pLabel_RepetitionRate;

    // Widgets for FLIm control // Laser control
    QCheckBox *m_pCheckBox_FlimLaserControl;
	QLabel *m_pLabel_FlimLaserPower;
	QSpinBox *m_pSpinBox_FlimLaserPower;

    // Widgets for FLIm control // Calibration
    QCheckBox *m_pCheckBox_AlazarDigitizerControl;
    QPushButton *m_pPushButton_FlimCalibration;
    FlimCalibDlg *m_pFlimCalibDlg;

    // Widgets for scanner control	
    QCheckBox *m_pCheckBox_GalvoScanControl;
	QLineEdit *m_pLineEdit_GalvoFastScanFreq;	
    QLineEdit *m_pLineEdit_PeakToPeakVoltage;
    QLineEdit *m_pLineEdit_OffsetVoltage;
	QLabel *m_pLabel_GalvoFastScanFreq;
	QLabel *m_pLabel_GalvoFastScanFreqHz;
    QLabel *m_pLabel_ScanVoltage;
    QLabel *m_pLabel_ScanPlusMinus;
    QLabel *m_pLabel_GalvanoVoltage;
    		
	// Widgets for Motorized stage control
	QCheckBox *m_pCheckBox_NanoscopeStageControl;
	
	QLabel *m_pLabel_StageUnit;

	QGroupBox *m_pGroupBox_Axis[3];

	QPushButton *m_pToggleButton_JogCW[3];
	QPushButton *m_pPushButton_RelCW[3];
	QPushButton *m_pPushButton_RelCCW[3];
	QPushButton *m_pToggleButton_JogCCW[3];
	QLabel *m_pLabel_Step[3];
	QLineEdit *m_pLineEdit_Step[3];
	QLabel *m_pLabel_Pos[3];
	QLineEdit *m_pLineEdit_Pos[3];	
	QLabel *m_pLabel_Speed[3];
	QLineEdit *m_pLineEdit_Speed[3];	
	QPushButton *m_pPushButton_Read[3];
	QPushButton *m_pPushButton_Stop[3];

	// Widgets for DPC illumination control
	QLabel *m_pLabel_Illumination;

	QRadioButton *m_pRadioButton_Off;
	QRadioButton *m_pRadioButton_BrightField;
	QRadioButton *m_pRadioButton_DarkField;
	QRadioButton *m_pRadioButton_Top;
	QRadioButton *m_pRadioButton_Bottom;
	QRadioButton *m_pRadioButton_Left;
	QRadioButton *m_pRadioButton_Right;	
	QButtonGroup *m_pButtonGroup_Illumination;
};

#endif // QDEVICECONTROLTAB_H
