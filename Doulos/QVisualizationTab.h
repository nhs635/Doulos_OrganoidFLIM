#ifndef QVISUALIZATIONTAB_H
#define QVISUALIZATIONTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>

#include <Common/medfilt.h>
#include <Common/ImageObject.h>
//#include <Common/basic_functions.h>

#include <iostream>
#include <vector>

#define FLIM_IMAGE_INTENSITY  -2
#define FLIM_IMAGE_LIFETIME   -3
#define FLIM_IMAGE_MERGED     -4

#define DPC_LIVE			  -5
#define DPC_PROCESSED		  -6


class QStreamTab;
class QResultTab;
class QImageView;


class QVisualizationTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QVisualizationTab(bool is_streaming = true, QWidget *parent = nullptr);
    virtual ~QVisualizationTab();

// Methods //////////////////////////////////////////////
public:
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
	inline QVBoxLayout* getVBoxLayoutVisWidgets() const { return m_pVBoxLayout_VisWidgets; }
    inline QGroupBox* getVisualizationWidgetsBox() const { return m_pGroupBox_VisualizationWidgets; }
    inline QGroupBox* getFlimVisualizationBox() const { return m_pGroupBox_FlimVisualization; }
	inline QGroupBox* getDpcImageModeBox() const { return m_pGroupBox_DpcImageMode; }
	inline QGroupBox* getDpcVisualizationBox() const { return m_pGroupBox_DpcVisualization; }
	inline QGroupBox* getQpiVisualizationBox() const { return m_pGroupBox_QpiRegularization; }
	inline QImageView* getImageView() const { return m_pImageView_Image; }
	inline medfilt* getMedfilt() const { return m_pMedfilt; }
	inline ImageObject* getImgObjIntensity() const { return m_pImgObjIntensity; }
	inline int getCurrentDpcImageMode() { return m_pButtonGroup_ImageModeDpc->checkedId(); }
	inline QRadioButton* getRadioButtonLive() { return m_pRadioButton_Live; }
	inline void setDpcImageMode(int mode) { changeDpcImageMode(mode); }

private:
    void createFlimVisualizationOptionTab();
	void createDpcVisualizationOptionTab();

public:
	void setImgViewVisPixelPos(bool vis);
	void getPixelPos(int* x, int* y);

public:
    void setObjects(int image_size, bool modality);

public slots:
    void visualizeImage(bool modality);

private slots:
    void changeFlimImageMode(int);
    void changeEmissionChannel(int);
    void changeLifetimeColorTable(int);
    void adjustFlimContrast();

	void changeDpcImageMode(int);
	void changeDpcColorTable(int);
	void changePhaseColorTable(int);
	void adjustDpcContrast();
	void adjustQpiRegParameters();

signals:
    void drawImage(bool);
	void plotImage(uint8_t*);
	void plotLiveImage(uint8_t*);
	void plotDpcTbImage(uint8_t*);
	void plotDpcLrImage(uint8_t*);
	void plotPhaseImage(uint8_t*);

// Variables ////////////////////////////////////////////
private:
	QStreamTab* m_pStreamTab;
	QResultTab* m_pResultTab;
    Configuration* m_pConfig;

public:
    // Visualization buffers
    std::vector<np::FloatArray2> m_vecVisIntensity;
    std::vector<np::FloatArray2> m_vecVisLifetime;

	np::FloatArray2 m_liveIntensity;
	std::vector<np::FloatArray2> m_vecIllumImages;

private:
    // Image visualization buffers
    ImageObject *m_pImgObjIntensity;
    ImageObject *m_pImgObjLifetime;
	ImageObject *m_pImgObjMerged;

	ImageObject *m_pImgObjLive;
	ImageObject *m_pImgObjDpcTb;
	ImageObject *m_pImgObjDpcLr;
	ImageObject *m_pImgObjPhase;

	medfilt* m_pMedfilt;

private:
    // Layout
    QVBoxLayout *m_pVBoxLayout;

    // Image viewer widgets
    QImageView *m_pImageView_Image;
	QImageView *m_pImageView_Dpc[4];

    // Visualization widgets groupbox
	QVBoxLayout *m_pVBoxLayout_VisWidgets;
    QGroupBox *m_pGroupBox_VisualizationWidgets;

    // FLIM visualization option widgets
    QGroupBox *m_pGroupBox_FlimVisualization;

    QLabel *m_pLabel_ImageMode;

    QRadioButton *m_pRadioButton_Intensity;
    QRadioButton *m_pRadioButton_Lifetime;
	QRadioButton *m_pRadioButton_Merged;

    QButtonGroup *m_pButtonGroup_ImageMode;

    QLabel *m_pLabel_EmissionChannel;
    QComboBox *m_pComboBox_EmissionChannel;

    QLabel *m_pLabel_LifetimeColorTable;
    QComboBox *m_pComboBox_LifetimeColorTable;

    QLabel *m_pLabel_NormIntensity;
    QLabel *m_pLabel_Lifetime;
    QLineEdit *m_pLineEdit_IntensityMax;
    QLineEdit *m_pLineEdit_IntensityMin;
    QLineEdit *m_pLineEdit_LifetimeMax;
    QLineEdit *m_pLineEdit_LifetimeMin;
    QImageView *m_pImageView_IntensityColorbar;
    QImageView *m_pImageView_LifetimeColorbar;

	// DPC visualization option widgets
	QGroupBox *m_pGroupBox_DpcImageMode;

	QLabel *m_pLabel_ImageModeDpc;

	QRadioButton *m_pRadioButton_Live;
	QRadioButton *m_pRadioButton_Processed;	
	QButtonGroup *m_pButtonGroup_ImageModeDpc;

	QGroupBox *m_pGroupBox_DpcVisualization;

	QLabel *m_pLabel_DpcColorTable;
	QComboBox *m_pComboBox_DpcColorTable;

	QLabel *m_pLabel_PhaseColorTable;
	QComboBox *m_pComboBox_PhaseColorTable;

	QLabel *m_pLabel_LiveIntensity;
	QLabel *m_pLabel_Dpc;	
	QLabel *m_pLabel_Phase;
	QLineEdit *m_pLineEdit_LiveIntensityMax;
	QLineEdit *m_pLineEdit_LiveIntensityMin;
	QLineEdit *m_pLineEdit_DpcMax;
	QLineEdit *m_pLineEdit_DpcMin;
	QLineEdit *m_pLineEdit_PhaseMax;
	QLineEdit *m_pLineEdit_PhaseMin;
	QImageView *m_pImageView_LiveIntensityColorbar;
	QImageView *m_pImageView_DpcColorbar;
	QImageView *m_pImageView_PhaseColorbar;

	QGroupBox *m_pGroupBox_QpiRegularization;

	QLabel *m_pLabel_L2Reg;
	QLineEdit *m_pLineEdit_L2RegAmp;
	QLineEdit *m_pLineEdit_L2RegPhase;
	QLabel *m_pLabel_TvReg;
	QLineEdit *m_pLineEdit_TvReg;
};

#endif // QVISUALIZATIONTAB_H
