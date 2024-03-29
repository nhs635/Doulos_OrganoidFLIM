#ifndef FLIMCALIBDLG_H
#define FLIMCALIBDLG_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Doulos/Configuration.h>
#include <Doulos/QDeviceControlTab.h>
#include <Doulos/Viewer/QScope.h>
#include <Doulos/Viewer/QImageView.h>

#include <Common/array.h>
#include <Common/callback.h>

#include <ipps.h>
#include <ippi.h>
#include <ippvm.h>

#define N_BINS 50

class QDeviceControlTab;
class FLImProcess;


class QMySpinBox : public QDoubleSpinBox
{
public:
	explicit QMySpinBox(QWidget *parent = nullptr) : QDoubleSpinBox(parent)
	{
		//lineEdit()->setReadOnly(true);
	}
	virtual ~QMySpinBox() {}
};

struct Histogram
{
public:
    Histogram() : lowerLevel(0), upperLevel(0), pHistObj(nullptr), pBuffer(nullptr), pLevels(nullptr)
    {
    }

    Histogram(int _nBins, int _length) : pHistObj(nullptr), pBuffer(nullptr), pLevels(nullptr), lowerLevel(0), upperLevel(0)
    {
        initialize(_nBins, _length);
    }

    ~Histogram()
    {
        if (pHistObj) ippsFree(pHistObj);
        if (pBuffer) ippsFree(pBuffer);
        if (pLevels) ippsFree(pLevels);
        if (pHistTemp) ippsFree(pHistTemp);
    }

public:
    void operator() (const Ipp32f* pSrc, Ipp32f* pHist, Ipp32f _lowerLevel, Ipp32f _upperLevel)
    {
        if ((lowerLevel != _lowerLevel) || (upperLevel != _upperLevel))
        {
            // set vars
            lowerLevel = _lowerLevel;
            upperLevel = _upperLevel;

            // initialize spec
            ippiHistogramUniformInit(ipp32f, &_lowerLevel, &_upperLevel, &nLevels, 1, pHistObj);

            // check levels of bins
            ippiHistogramGetLevels(pHistObj, &pLevels);
        }

        // calculate histogram
        ippiHistogram_32f_C1R(pSrc, sizeof(Ipp32f) * roiSize.width, roiSize, pHistTemp, pHistObj, pBuffer);
        ippsConvert_32s32f((Ipp32s*)pHistTemp, pHist, nBins);
    }

public:
    void initialize(int _nBins, int _length)
    {
        // init vars
        roiSize = { _length, 1 };
        nBins = _nBins; nLevels = nBins + 1;
        pLevels = ippsMalloc_32f(nLevels);

        // get sizes for spec and buffer
        ippiHistogramGetBufferSize(ipp32f, roiSize, &nLevels, 1/*nChan*/, 1/*uniform*/, &sizeHistObj, &sizeBuffer);

        pHistObj = (IppiHistogramSpec*)ippsMalloc_8u(sizeHistObj);
        pBuffer = (Ipp8u*)ippsMalloc_8u(sizeBuffer);
        pHistTemp = ippsMalloc_32u(nBins);
    }

private:
    IppiSize roiSize;
    int nBins, nLevels;
    int sizeHistObj, sizeBuffer;
    Ipp32f lowerLevel, upperLevel;
    IppiHistogramSpec* pHistObj;
    Ipp8u* pBuffer;
    Ipp32f* pLevels;
    Ipp32u* pHistTemp;
};


class FlimCalibDlg : public QDialog
{
    Q_OBJECT

    // Constructer & Destructer /////////////////////////////
public:
    explicit FlimCalibDlg(QWidget *parent = nullptr);
    virtual ~FlimCalibDlg();

    // Methods //////////////////////////////////////////////
private:
    void keyPressEvent(QKeyEvent *e);

private:
    void createPulseView();
    void createCalibWidgets();
    void createHistogram();

public:
	inline QImageView* getPulseImageView() const { return m_pImageView_PulseImage; }
	inline bool getRoiSegmentViewEnabled() { return m_pCheckBox_RoiSegmentView->isChecked(); }

private: // callbacks


public slots : // widgets
    void drawRoiPulse(FLImProcess*, int);
	void drawRoiPulse(float*, int);
	void drawHistogram(float*, float*);

    void showWindow(bool);
    void showMeanDelay(bool);
    void splineView(bool);

	void enableRoiSegmentView(bool);

	void writeCurrentPulses(bool);

    void captureBackground();
    void captureBackground(const QString &);

    void resetChStart0(double);
    void resetChStart1(double);
    void resetChStart2(double);
    void resetChStart3(double);
    void resetDelayTimeOffset();

signals:
    void plotRoiPulse(FLImProcess*, int);
	void plotRoiPulse(float*, int);
	void plotHistogram(float*, float*);

    // Variables ////////////////////////////////////////////
private:
    Configuration* m_pConfig;
    QDeviceControlTab* m_pDeviceControlTab;
    FLImProcess* m_pFLIm;

private:
    Histogram* m_pHistogramIntensity;
    Histogram* m_pHistogramLifetime;

private:
    // Layout
    QVBoxLayout *m_pVBoxLayout;

    // Widgets for pulse view
	QImageView *m_pImageView_PulseImage;
    QScope *m_pScope_PulseView;

    QCheckBox *m_pCheckBox_ShowWindow;
    QCheckBox *m_pCheckBox_ShowMeanDelay;
    QCheckBox *m_pCheckBox_SplineView;
	QCheckBox *m_pCheckBox_RoiSegmentView;

    // Widgets for FLIM calibration widgets
	QPushButton *m_pToggleButton_WriteCurrentPulses;

    QPushButton *m_pPushButton_CaptureBackground;
    QLineEdit *m_pLineEdit_Background;

    QLabel *m_pLabel_ChStart;
    QLabel *m_pLabel_DelayTimeOffset;
    QLabel *m_pLabel_Ch[4];
    QMySpinBox *m_pSpinBox_ChStart[4];
    QLineEdit *m_pLineEdit_DelayTimeOffset[3];
    QLabel *m_pLabel_NanoSec[2];

    // Widgets for histogram
    QLabel *m_pLabel_FluIntensity;
    QRenderArea *m_pRenderArea_FluIntensity;
    QImageView *m_pColorbar_FluIntensity;
    QLabel *m_pLabel_FluIntensityMin;
    QLabel *m_pLabel_FluIntensityMax;
    QLabel *m_pLabel_FluIntensityMean;
    QLabel *m_pLabel_FluIntensityStd;

    QLabel *m_pLabel_FluLifetime;
    QRenderArea *m_pRenderArea_FluLifetime;
    QImageView *m_pColorbar_FluLifetime;
    QLabel *m_pLabel_FluLifetimeMin;
    QLabel *m_pLabel_FluLifetimeMax;
    QLabel *m_pLabel_FluLifetimeMean;
    QLabel *m_pLabel_FluLifetimeStd;
};

#endif // FLIMCALIBDLG_H
