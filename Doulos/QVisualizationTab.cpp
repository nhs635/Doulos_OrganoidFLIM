
#include "QVisualizationTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>

#include <Doulos/Dialog/FlimCalibDlg.h>
#include <Doulos/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>


QVisualizationTab::QVisualizationTab(bool is_streaming, QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr),
    m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr), m_pImgObjMerged(nullptr), m_pMedfilt(nullptr)
{
    // Set configuration objects
	if (is_streaming)
	{
		m_pStreamTab = (QStreamTab*)parent;
		m_pConfig = m_pStreamTab->getMainWnd()->m_pConfiguration;
	}
	else
	{
		//m_pResultTab = (QResultTab*)parent;
	}

    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(1);

    // Create image view
    m_pImageView_Image = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), m_pConfig->nPixels, m_pConfig->nLines);
	m_pImageView_Image->setMinimumSize(500, 500);
    m_pImageView_Image->setSquare(true);
	m_pImageView_Image->setMovedMouseCallback([&](QPoint& p) { m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	m_pImageView_Image->setClickedMouseCallback([&](int x, int y) {
			m_pImageView_Image->getRender()->m_pixelPos[0] = x;
			m_pImageView_Image->getRender()->m_pixelPos[1] = y;
			m_pImageView_Image->getRender()->update();
	});

    // Create FLIM visualization option tab
    createFlimVisualizationOptionTab();

    // Set layout
    m_pGroupBox_VisualizationWidgets = new QGroupBox;
    m_pGroupBox_VisualizationWidgets->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pGroupBox_VisualizationWidgets->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");

    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->setSpacing(1);

    pVBoxLayout->addWidget(m_pGroupBox_FlimVisualization);

    m_pGroupBox_VisualizationWidgets->setLayout(pVBoxLayout);

    m_pVBoxLayout->addWidget(m_pImageView_Image);

    setLayout(m_pVBoxLayout);

    // Connect signal and slot
    connect(this, SIGNAL(drawImage()), this, SLOT(visualizeImage()));
	connect(this, SIGNAL(plotImage(uint8_t*)), m_pImageView_Image, SLOT(drawImage(uint8_t*)));
}

QVisualizationTab::~QVisualizationTab()
{
    if (m_pImgObjIntensity) delete m_pImgObjIntensity;
    if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	if (m_pImgObjMerged) delete m_pImgObjMerged;
	
    if (m_pMedfilt) delete m_pMedfilt;
}


void QVisualizationTab::createFlimVisualizationOptionTab()
{
    // Create widgets for FLIM visualization option tab
    m_pGroupBox_FlimVisualization = new QGroupBox;
    m_pGroupBox_FlimVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_pGroupBox_FlimVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
    QGridLayout *pGridLayout_FlimVisualization = new QGridLayout;
    pGridLayout_FlimVisualization->setSpacing(3);

    // Create widgets for image mode
    m_pLabel_ImageMode = new QLabel(this);
    m_pLabel_ImageMode->setText("Image Mode   ");

    m_pRadioButton_Intensity = new QRadioButton(this);
    m_pRadioButton_Intensity->setText("Intensity");
    m_pRadioButton_Lifetime = new QRadioButton(this);
    m_pRadioButton_Lifetime->setText("Lifetime");
	m_pRadioButton_Merged = new QRadioButton(this);
	m_pRadioButton_Merged->setText("Merged");

    m_pButtonGroup_ImageMode = new QButtonGroup(this);
    m_pButtonGroup_ImageMode->addButton(m_pRadioButton_Intensity, FLIM_IMAGE_INTENSITY);
    m_pButtonGroup_ImageMode->addButton(m_pRadioButton_Lifetime, FLIM_IMAGE_LIFETIME);
	m_pButtonGroup_ImageMode->addButton(m_pRadioButton_Merged, FLIM_IMAGE_MERGED);
    m_pRadioButton_Intensity->setChecked(true);

    // Create widgets for FLIM emission control
    m_pComboBox_EmissionChannel = new QComboBox(this);
    m_pComboBox_EmissionChannel->addItem("Ch 1");
    m_pComboBox_EmissionChannel->addItem("Ch 2");
    m_pComboBox_EmissionChannel->addItem("Ch 3");
	m_pComboBox_EmissionChannel->addItem("Ch 4");
    m_pComboBox_EmissionChannel->setCurrentIndex(m_pConfig->flimEmissionChannel - 1);
    m_pLabel_EmissionChannel = new QLabel("Em Channel  ", this);
    m_pLabel_EmissionChannel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    m_pLabel_EmissionChannel->setBuddy(m_pComboBox_EmissionChannel);

    // Create widgets for FLIM lifetime colortable selection
    ColorTable temp_ctable;
    m_pComboBox_LifetimeColorTable = new QComboBox(this);
    for (int i = 0; i < temp_ctable.m_cNameVector.size(); i++)
        m_pComboBox_LifetimeColorTable->addItem(temp_ctable.m_cNameVector.at(i));
    m_pComboBox_LifetimeColorTable->setCurrentIndex(m_pConfig->flimLifetimeColorTable);
    m_pLabel_LifetimeColorTable = new QLabel("    Lifetime Colortable  ", this);
    m_pLabel_LifetimeColorTable->setBuddy(m_pComboBox_LifetimeColorTable);

    // Create line edit widgets for FLIM contrast adjustment
    m_pLineEdit_IntensityMax = new QLineEdit(this);
    m_pLineEdit_IntensityMax->setFixedWidth(30);
    m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    m_pLineEdit_IntensityMax->setAlignment(Qt::AlignCenter);
    m_pLineEdit_IntensityMin = new QLineEdit(this);
    m_pLineEdit_IntensityMin->setFixedWidth(30);
    m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
    m_pLineEdit_IntensityMin->setAlignment(Qt::AlignCenter);
    m_pLineEdit_LifetimeMax = new QLineEdit(this);
    m_pLineEdit_LifetimeMax->setFixedWidth(30);
    m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max, 'f', 1));
    m_pLineEdit_LifetimeMax->setAlignment(Qt::AlignCenter);
    m_pLineEdit_LifetimeMin = new QLineEdit(this);
    m_pLineEdit_LifetimeMin->setFixedWidth(30);
    m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 'f', 1));
    m_pLineEdit_LifetimeMin->setAlignment(Qt::AlignCenter);

    // Create color bar for FLIM visualization
    uint8_t color[256];
    for (int i = 0; i < 256; i++)
        color[i] = i;

    m_pImageView_IntensityColorbar = new QImageView(ColorTable::colortable(INTENSITY_COLORTABLE), 256, 1);
    m_pImageView_IntensityColorbar->setFixedSize(150, 15);
    m_pImageView_IntensityColorbar->drawImage(color);
    m_pImageView_LifetimeColorbar = new QImageView(ColorTable::colortable(m_pConfig->flimLifetimeColorTable), 256, 1);
    m_pImageView_LifetimeColorbar->setFixedSize(150, 15);
    m_pImageView_LifetimeColorbar->drawImage(color);
	m_pLabel_NormIntensity = new QLabel(QString("Ch%1 Intensity ").arg(m_pConfig->flimEmissionChannel), this);
	m_pLabel_NormIntensity->setFixedWidth(70);
	m_pLabel_NormIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_pLabel_Lifetime = new QLabel(QString("Ch%1 Lifetime ").arg(m_pConfig->flimEmissionChannel), this);
	m_pLabel_Lifetime->setFixedWidth(70);
	m_pLabel_Lifetime->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    // Set layout
    QHBoxLayout *pHBoxLayout_FlimVisualization1 = new QHBoxLayout;
    pHBoxLayout_FlimVisualization1->addWidget(m_pLabel_ImageMode);
	pHBoxLayout_FlimVisualization1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    pHBoxLayout_FlimVisualization1->addWidget(m_pRadioButton_Intensity);
	pHBoxLayout_FlimVisualization1->addWidget(m_pRadioButton_Lifetime);
	pHBoxLayout_FlimVisualization1->addWidget(m_pRadioButton_Merged);

    QHBoxLayout *pHBoxLayout_FlimVisualization2 = new QHBoxLayout;
    pHBoxLayout_FlimVisualization2->addWidget(m_pLabel_EmissionChannel);
    pHBoxLayout_FlimVisualization2->addWidget(m_pComboBox_EmissionChannel);
    pHBoxLayout_FlimVisualization2->addWidget(m_pLabel_LifetimeColorTable);
    pHBoxLayout_FlimVisualization2->addWidget(m_pComboBox_LifetimeColorTable);

    QHBoxLayout *pHBoxLayout_IntensityColorbar = new QHBoxLayout;
    pHBoxLayout_IntensityColorbar->setSpacing(3);
    pHBoxLayout_IntensityColorbar->addWidget(m_pLabel_NormIntensity);
    pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMin);
    pHBoxLayout_IntensityColorbar->addWidget(m_pImageView_IntensityColorbar);
    pHBoxLayout_IntensityColorbar->addWidget(m_pLineEdit_IntensityMax);

    QHBoxLayout *pHBoxLayout_LifetimeColorbar = new QHBoxLayout;
    pHBoxLayout_LifetimeColorbar->setSpacing(3);
    pHBoxLayout_LifetimeColorbar->addWidget(m_pLabel_Lifetime);
    pHBoxLayout_LifetimeColorbar->addWidget(m_pLineEdit_LifetimeMin);
    pHBoxLayout_LifetimeColorbar->addWidget(m_pImageView_LifetimeColorbar);
    pHBoxLayout_LifetimeColorbar->addWidget(m_pLineEdit_LifetimeMax);

    pGridLayout_FlimVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
    pGridLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization1, 0, 1);
    pGridLayout_FlimVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_FlimVisualization->addItem(pHBoxLayout_FlimVisualization2, 1, 1);
    pGridLayout_FlimVisualization->addItem(pHBoxLayout_IntensityColorbar, 2, 0, 1, 2);
    pGridLayout_FlimVisualization->addItem(pHBoxLayout_LifetimeColorbar, 3, 0, 1, 2);

    m_pGroupBox_FlimVisualization->setLayout(pGridLayout_FlimVisualization);

    // Connect signal and slot
    connect(m_pButtonGroup_ImageMode, SIGNAL(buttonClicked(int)), this, SLOT(changeImageMode(int)));
    connect(m_pComboBox_EmissionChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));
    connect(m_pComboBox_LifetimeColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLifetimeColorTable(int)));
    connect(m_pLineEdit_IntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
    connect(m_pLineEdit_IntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
    connect(m_pLineEdit_LifetimeMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
    connect(m_pLineEdit_LifetimeMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
}

void QVisualizationTab::setImgViewVisPixelPos(bool vis)
{
	m_pImageView_Image->getRender()->setVisPixelPos(vis);
}

void QVisualizationTab::getPixelPos(int* x, int* y)
{
	*x = m_pImageView_Image->getRender()->m_pixelPos[0];
	*y = m_pImageView_Image->getRender()->m_pixelPos[1];
}


void QVisualizationTab::setObjects(int n_lines)
{
	int id = m_pButtonGroup_ImageMode->checkedId();

    // Clear existed buffers
    std::vector<np::FloatArray2> clear_vector1;
    clear_vector1.swap(m_vecVisIntensity);
    std::vector<np::FloatArray2> clear_vector2;
    clear_vector2.swap(m_vecVisLifetime);

    // Reset image view size
	if (id == FLIM_IMAGE_INTENSITY)
	{
		m_pImageView_Image->resetColormap(ColorTable::colortable(INTENSITY_COLORTABLE));
		m_pImageView_Image->resetSize(m_pConfig->nPixels, n_lines, false);
	}
	else if (id == FLIM_IMAGE_LIFETIME)
	{
		m_pImageView_Image->resetColormap(ColorTable::colortable(m_pConfig->flimLifetimeColorTable));
		m_pImageView_Image->resetSize(m_pConfig->nPixels, n_lines, false);
	}
	else if (id == FLIM_IMAGE_MERGED)
	{
		m_pImageView_Image->resetSize(m_pConfig->nPixels, n_lines, true);
	}

    // Create visualization buffers
    for (int i = 0; i < 4; i++)
    {
        np::FloatArray2 intensity = np::FloatArray2(m_pConfig->nPixels, n_lines);
        np::FloatArray2 lifetime = np::FloatArray2(m_pConfig->nPixels, n_lines);
        m_vecVisIntensity.push_back(intensity);
        m_vecVisLifetime.push_back(lifetime);
    }
	
    // Create image visualization buffers
    ColorTable temp_ctable;
    if (m_pImgObjIntensity) delete m_pImgObjIntensity;
    m_pImgObjIntensity = new ImageObject(m_pConfig->nPixels, n_lines, temp_ctable.m_colorTableVector.at(INTENSITY_COLORTABLE));
    if (m_pImgObjLifetime) delete m_pImgObjLifetime;
    m_pImgObjLifetime = new ImageObject(m_pConfig->nPixels, n_lines, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));
	if (m_pImgObjMerged) delete m_pImgObjMerged;
	m_pImgObjMerged = new ImageObject(m_pConfig->nPixels, n_lines, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));
    if (m_pMedfilt) delete m_pMedfilt;
    m_pMedfilt = new medfilt(m_pConfig->nPixels, n_lines, 3, 3);
}


void QVisualizationTab::visualizeImage()
{
	int id = m_pButtonGroup_ImageMode->checkedId();

    IppiSize roi_flim = { m_pConfig->nPixels, m_pConfig->nLines };

	// Intensity Image
	float* scanIntensity = m_vecVisIntensity.at(m_pConfig->flimEmissionChannel - 1).raw_ptr();
	ippiScale_32f8u_C1R(scanIntensity, sizeof(float) * roi_flim.width, m_pImgObjIntensity->arr.raw_ptr(), sizeof(uint8_t) * roi_flim.width,
		roi_flim, m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min, 
		m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max);
	//ippiTranspose_8u_C1IR(m_pImgObjIntensity->arr.raw_ptr(), roi_flim.width, roi_flim);
	(*m_pMedfilt)(m_pImgObjIntensity->arr.raw_ptr());

	// Lifetime Image
	float* scanLifetime = m_vecVisLifetime.at(m_pConfig->flimEmissionChannel - 1).raw_ptr();
	ippiScale_32f8u_C1R(scanLifetime, sizeof(float) * roi_flim.width, m_pImgObjLifetime->arr.raw_ptr(), sizeof(uint8_t) * roi_flim.width,
		roi_flim, m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min, 
		m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max);
	//ippiTranspose_8u_C1IR(m_pImgObjLifetime->arr.raw_ptr(), roi_flim.width, roi_flim);
	(*m_pMedfilt)(m_pImgObjLifetime->arr.raw_ptr());

	// Non HSV intensity-weight map
	if (id == FLIM_IMAGE_MERGED)
	{
		ColorTable temp_ctable;
		ImageObject tempImgObj(m_pImgObjMerged->getWidth(), m_pImgObjMerged->getHeight(), temp_ctable.m_colorTableVector.at(ColorTable::gray));

		m_pImgObjLifetime->convertRgb();
		memcpy(tempImgObj.qindeximg.bits(), m_pImgObjIntensity->arr.raw_ptr(), tempImgObj.qindeximg.byteCount());
		tempImgObj.convertRgb();

		ippsMul_8u_Sfs(m_pImgObjLifetime->qrgbimg.bits(), tempImgObj.qrgbimg.bits(), m_pImgObjMerged->qrgbimg.bits(), tempImgObj.qrgbimg.byteCount(), 8);
	}
	
	// Visualization
	if (id == FLIM_IMAGE_INTENSITY)
		emit plotImage(m_pImgObjIntensity->qindeximg.bits());
	else if (id == FLIM_IMAGE_LIFETIME)
		emit plotImage(m_pImgObjLifetime->qindeximg.bits());
	else if (id == FLIM_IMAGE_MERGED)
		emit plotImage(m_pImgObjMerged->qrgbimg.bits());
}


void QVisualizationTab::changeImageMode(int mode)
{
	if (mode == FLIM_IMAGE_INTENSITY)
	{
		m_pImageView_Image->resetColormap(ColorTable::colortable(INTENSITY_COLORTABLE));
		m_pImageView_Image->resetSize(m_pConfig->nPixels, m_pConfig->nLines, false);
	}
	else if (mode == FLIM_IMAGE_LIFETIME)
	{
		m_pImageView_Image->resetColormap(ColorTable::colortable(m_pConfig->flimLifetimeColorTable));
		m_pImageView_Image->resetSize(m_pConfig->nPixels, m_pConfig->nLines, false);
	}
	else if (mode == FLIM_IMAGE_MERGED)
	{
		m_pImageView_Image->resetSize(m_pConfig->nPixels, m_pConfig->nLines, true);
	}

    visualizeImage();
}

void QVisualizationTab::changeEmissionChannel(int ch)
{
    m_pConfig->flimEmissionChannel = ch + 1;

	// Set labels
	m_pLabel_NormIntensity->setText(QString("Ch%1 Intensity ").arg(ch + 1));
	m_pLabel_Lifetime->setText(QString("Ch%1 Lifetime ").arg(ch + 1));

	// Set line edits
	m_pLineEdit_IntensityMin->setText(QString::number(m_pConfig->flimIntensityRange[ch].min, 'f', 1));
	m_pLineEdit_IntensityMax->setText(QString::number(m_pConfig->flimIntensityRange[ch].max, 'f', 1));
	m_pLineEdit_LifetimeMin->setText(QString::number(m_pConfig->flimLifetimeRange[ch].min, 'f', 1));
	m_pLineEdit_LifetimeMax->setText(QString::number(m_pConfig->flimLifetimeRange[ch].max, 'f', 1));

    // Transfer to FLIm calibration dlg
	QDeviceControlTab* pDeviceControlTab = m_pStreamTab->getDeviceControlTab();
	FLImProcess* pFLIm = m_pStreamTab->getOperationTab()->getDataAcq()->getFLIm();

	if (m_pStreamTab)
	    if (pDeviceControlTab->getFlimCalibDlg())
		    emit pDeviceControlTab->getFlimCalibDlg()->plotRoiPulse(pFLIm, 0);

    visualizeImage();
}

void QVisualizationTab::changeLifetimeColorTable(int ctable_ind)
{
    m_pConfig->flimLifetimeColorTable = ctable_ind;

    if (m_pButtonGroup_ImageMode->checkedId() == FLIM_IMAGE_LIFETIME)
        m_pImageView_Image->resetColormap(ColorTable::colortable(ctable_ind));

    m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable(ctable_ind));
    m_pImageView_LifetimeColorbar->resetColormap(ColorTable::colortable(ctable_ind));

    ColorTable temp_ctable;
    if (m_pImgObjLifetime) delete m_pImgObjLifetime;
    m_pImgObjLifetime = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(ctable_ind));
	if (m_pImgObjMerged) delete m_pImgObjMerged;
	m_pImgObjMerged = new ImageObject(m_pConfig->nPixels, m_pConfig->nLines, temp_ctable.m_colorTableVector.at(m_pConfig->flimLifetimeColorTable));

    visualizeImage();
}

void QVisualizationTab::adjustFlimContrast()
{
    m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
    m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
    m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
    m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();

    visualizeImage();
}
