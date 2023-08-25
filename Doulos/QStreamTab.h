#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>

#include <Common/array.h>
#include <Common/SyncObject.h>

class MainWindow;
class QOperationTab;
class QDeviceControlTab;
class QVisualizationTab;

class ThreadManager;
class FLImProcess;

#define MODALITY_FLIM		0
#define MODALITY_DPC		1

#define N_LINES_126			126
#define N_LINES_250			250
#define N_LINES_500			500
#define N_LINES_1000		1000


class QStreamTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QStreamTab(QWidget *parent = nullptr);
	virtual ~QStreamTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
    inline QOperationTab* getOperationTab() const { return m_pOperationTab; }
    inline QDeviceControlTab* getDeviceControlTab() const { return m_pDeviceControlTab; }
	inline QVisualizationTab* getVisualizationTab() const { return m_pVisualizationTab; }

	inline size_t getFlimProcessingBufferQueueSize() { return m_syncFlimProcessing.queue_buffer.size(); }
	inline size_t getFlimVisualizationBufferQueueSize() { return m_syncFlimVisualization.queue_buffer.size(); }

	inline bool getCurrentModality() { return m_pRadioButton_FLIM->isChecked(); }

	inline QCheckBox* getImageStitchingCheckBox() const { return m_pCheckBox_StitchingMode; }
	inline QLabel* getXStepLabel() const { return m_pLabel_XStep; }
	inline QLabel* getYStepLabel() const { return m_pLabel_YStep; }
	inline QLineEdit* getXStepLineEdit() const { return m_pLineEdit_XStep; }
	inline QLineEdit* getYStepLineEdit() const { return m_pLineEdit_YStep; }
	
public:
    void setWidgetsText();
    void setYLinesWidgets(bool enabled);
	void setAveragingWidgets(bool enabled);
	void updateCmosGainExposure();

private:		
// Set thread callback objects
    void setFlimAcquisitionCallback();
    void setFlimProcessingCallback();
	void setDpcAcquisitionCallback();
    void setVisualizationCallback();

private slots:
	void onTimerMonitoring();
	void changeModality(int);
    void changeYLines(int);
	void changeAveragingFrame(const QString &);
	void changeBiDirScanComp(double);
	void autoCrsCompSet();
	void changeCRSNonlinearityComp(bool);
	void changeCmosGain(int);
	void changeCmosExposure(int);
	void enableStitchingMode(bool);
	void changeStitchingXStep(const QString &);
	void changeStitchingYStep(const QString &);
    void processMessage(QString, bool);

signals:
    void sendStatusMessage(QString, bool);
	void setAcquisitionStatus(QString);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
    Configuration* m_pConfig;

    QOperationTab* m_pOperationTab;
    QDeviceControlTab* m_pDeviceControlTab;
    QVisualizationTab* m_pVisualizationTab;

	// Message Window
	QListWidget *m_pListWidget_MsgWnd;

public:
	// Writing state & flag
	int m_nWrittenSamples;
	int m_nAcquiredFrames;
	int m_nAverageCount;
	bool m_bRecordingPhase;

	// CRS nonlinearity compensation idx
	np::FloatArray2 m_pCRSCompIdx;

	// Stitching flag
	bool m_bIsStageTransition;
	bool m_bIsStageTransited;
	int m_nImageCount;

public:
	// Image buffer
	np::FloatArray2 m_pTempIntensity;
	np::FloatArray2 m_pTempLifetime;
	np::FloatArray2 m_pNonNaNIndex;

public:
    // Thread manager objects
    ThreadManager* m_pThreadFlimProcess;
    ThreadManager* m_pThreadVisualization;

private:
    // Thread synchronization objects
    SyncObject<float> m_syncFlimProcessing;
    SyncObject<float> m_syncFlimVisualization;
	SyncObject<uint16_t> m_syncDpcProcessing;

	// Monitoring timer
	QTimer *m_pTimer_Monitoring;
	
private:
    // Layout
    QHBoxLayout *m_pHBoxLayout;

    // Group Boxes	
    QGroupBox *m_pGroupBox_OperationTab;
    QGroupBox *m_pGroupBox_DeviceControlTab;
    QGroupBox *m_pGroupBox_VisualizationTab;
    QGroupBox *m_pGroupBox_ImageSizeTab;
	QGroupBox *m_pGroupBox_ModalityTab;
	QGroupBox *m_pGroupBox_CmosProperties;
	QGroupBox *m_pGroupBox_StitchingTab;

	// Modality radio buttons
	QLabel *m_pLabel_Modality;
	QRadioButton *m_pRadioButton_FLIM;
	QRadioButton *m_pRadioButton_DPC;
	QButtonGroup *m_pButtonGroup_Modality;

    // Y lines radio buttons
    QLabel *m_pLabel_YLines;
    QRadioButton *m_pRadioButton_126;
    QRadioButton *m_pRadioButton_250;
    QRadioButton *m_pRadioButton_500;
    QRadioButton *m_pRadioButton_1000;
    QButtonGroup *m_pButtonGroup_YLines;

	// Image averaging mode 
	QLabel *m_pLabel_Averaging;
	QLineEdit *m_pLineEdit_Averaging;

	QLabel *m_pLabel_AcquisitionStatus;
	
#if (CRS_DIR_FACTOR == 2)
	// Bidirectional scan compensation
	QLabel *m_pLabel_BiDirScanComp;
	QDoubleSpinBox *m_pDoubleSpinBox_BiDirScanComp;
	QPushButton *m_pPushButton_AutoCorr;
#endif

	// CRS nonlinearity compensation
	QCheckBox *m_pCheckBox_CRSNonlinearityComp;

	// CMOS gain & exposure
	QLabel *m_pLabel_Gain;
	QSlider *m_pSlider_Gain;
	QLabel *m_pLabel_Exposure;
	QSlider *m_pSlider_Exposure;
	
	// Image stitching mode
	QCheckBox *m_pCheckBox_StitchingMode;
	QLineEdit *m_pLineEdit_XStep;
	QLineEdit *m_pLineEdit_YStep;
	QLabel *m_pLabel_XStep;
	QLabel *m_pLabel_YStep;

	// Image z-stack mode
	// .. to be added
};

#endif // QSTREAMTAB_H
