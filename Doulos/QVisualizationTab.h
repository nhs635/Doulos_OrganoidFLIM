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
    inline QGroupBox* getVisualizationWidgetsBox() const { return m_pGroupBox_VisualizationWidgets; }
    inline QGroupBox* getFlimVisualizationBox() const { return m_pGroupBox_FlimVisualization; }
	inline QImageView* getImageView() const { return m_pImageView_Image; }
	inline medfilt* getMedfilt() const { return m_pMedfilt; }

private:
    void createFlimVisualizationOptionTab();

public:
	void setImgViewVisPixelPos(bool vis);
	void getPixelPos(int* x, int* y);

public:
    void setObjects(int image_size);

public slots:
    void visualizeImage();

private slots:
    void changeImageMode(int);
    void changeEmissionChannel(int);
    void changeLifetimeColorTable(int);
    void adjustFlimContrast();

signals:
    void drawImage();
	void plotImage(uint8_t*);

// Variables ////////////////////////////////////////////
private:
	QStreamTab* m_pStreamTab;
	QResultTab* m_pResultTab;
    Configuration* m_pConfig;

public:
    // Visualization buffers
    std::vector<np::FloatArray2> m_vecVisIntensity;
    std::vector<np::FloatArray2> m_vecVisLifetime;

private:
    // Image visualization buffers
    ImageObject *m_pImgObjIntensity;
    ImageObject *m_pImgObjLifetime;
	ImageObject *m_pImgObjMerged;

	medfilt* m_pMedfilt;

private:
    // Layout
    QVBoxLayout *m_pVBoxLayout;

    // Image viewer widgets
    QImageView *m_pImageView_Image;

    // Visualization widgets groupbox
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
};

#endif // QVISUALIZATIONTAB_H
