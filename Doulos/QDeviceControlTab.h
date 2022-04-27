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

class ResonantScan;
class GalvoScan;
#if (CRS_DIR_FACTOR == 2)
class TwoEdgeTriggerEnable;
#endif
class ZaberStage;

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

	inline QCheckBox* getPmtGainControl() const { return m_pCheckBox_PmtGainControl; }
	inline QCheckBox* getFlimLaserTrigControl() const { return m_pCheckBox_FlimLaserTrigControl; }
	inline QCheckBox* getFlimLaserControl() const { return m_pCheckBox_FlimLaserControl; }

    inline FlimCalibDlg* getFlimCalibDlg() const { return m_pFlimCalibDlg; }
    inline QCheckBox* getAlazarDigitizerControl() const { return m_pCheckBox_AlazarDigitizerControl; }	
	
	inline QCheckBox* getResonantScanControl() const { return m_pCheckBox_ResonantScanControl; }
	inline QPushButton* getResonantScanVoltageControl() const { return m_pToggledButton_ResonantScan; }
	inline QDoubleSpinBox* getResonantScanVoltageSpinBox() const { return m_pDoubleSpinBox_ResonantScan; }
	inline ResonantScan* getResonantScan() const { return m_pResonantScan; }
    inline QCheckBox* getGalvoScanControl() const { return m_pCheckBox_GalvoScanControl; }
	inline GalvoScan* getGalvoScan() const { return m_pGalvoScan; }

	inline QCheckBox* getZaberStageControl() const { return m_pCheckBox_ZaberStageControl; }
	inline ZaberStage* getZaberStage() const { return m_pZaberStage; }

private: ////////////////////////////////////////////////////////////////////////////////////////////////
    void createPmtGainControl();
    void createFlimTriggeringControl();
	void createFlimLaserControl();
    void createFlimCalibControl();
    void createScanControl();
	void createZaberStageControl();

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
	void connectResonantScanner(bool);
	void applyResonantScannerVoltage(bool);
	void setResonantScannerVoltage(double);

    void connectGalvanoMirror(bool);
    void changeGalvoScanVoltage(const QString &);
    void changeGalvoScanVoltageOffset(const QString &);

	// Zaber Stage Control
	void connectZaberStage(bool);
	void moveRelative();
	void setTargetSpeed(const QString &);
	void changeZaberPullbackLength(const QString &);
	void home();
	void stop();
	void stageScan(int, double);
	//void getCurrentPosition();

signals: ////////////////////////////////////////////////////////////////////////////////////////////////
	void startStageScan(int, double);
	void monitoring();

private: ////////////////////////////////////////////////////////////////////////////////////////////////
    // PMT Gain Control & FLIm Laser Triggering
    PmtGainControl* m_pPmtGainControl;
    FlimTrigger* m_pFlimTrigControl; 

	// FLIm pulsed Laser Control
	IPGPhotonicsLaser* m_pIPGPhotonicsLaser;

    // Resonant & Galvo Scanner Control
	ResonantScan* m_pResonantScan;
    GalvoScan* m_pGalvoScan;	
#if (CRS_DIR_FACTOR == 2)
	TwoEdgeTriggerEnable* m_pTwoEdgeTriggerEnable;
#endif

	// Zaber Stage Control;
	ZaberStage* m_pZaberStage;
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
    QStreamTab* m_pStreamTab;
    Configuration* m_pConfig;

    // Layout
    QVBoxLayout *m_pVBoxLayout;

	// FLIM Layout
	QVBoxLayout *m_pVBoxLayout_FlimControl;
    QGroupBox *m_pGroupBox_FlimControl;

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
	QCheckBox *m_pCheckBox_ResonantScanControl;
	QPushButton *m_pToggledButton_ResonantScan;
	QDoubleSpinBox *m_pDoubleSpinBox_ResonantScan;
	QLabel *m_pLabel_ResonantScan;

    QCheckBox *m_pCheckBox_GalvoScanControl;
    QLineEdit *m_pLineEdit_PeakToPeakVoltage;
    QLineEdit *m_pLineEdit_OffsetVoltage;
    QLabel *m_pLabel_ScanVoltage;
    QLabel *m_pLabel_ScanPlusMinus;
    QLabel *m_pLabel_GalvanoVoltage;
    		
	// Widgets for Zaber stage control
	QCheckBox *m_pCheckBox_ZaberStageControl;
	QPushButton *m_pPushButton_SetStageNumber;
	QPushButton *m_pPushButton_SetTargetSpeed;
	QPushButton *m_pPushButton_MoveRelative;
	QPushButton *m_pPushButton_Home;
	QPushButton *m_pPushButton_Stop;
	QComboBox *m_pComboBox_StageNumber;
	QLineEdit *m_pLineEdit_TargetSpeed;
	QLineEdit *m_pLineEdit_TravelLength;
	QLabel *m_pLabel_TargetSpeed;
	QLabel *m_pLabel_TravelLength;
};

#endif // QDEVICECONTROLTAB_H
