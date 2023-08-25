
#include "QVisualizationTab.h"

#include <Doulos/MainWindow.h>
#include <Doulos/QStreamTab.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>

#include <Doulos/Dialog/FlimCalibDlg.h>
#include <Doulos/Viewer/QImageView.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>
#include <DataAcquisition/QpiProcess/QpiProcess.h>
#include <DataAcquisition/ImagingSource/ImagingSource.h>

#include <DeviceControl/DpcIllumination/DpcIllumination.h>

#include <ippcore.h>
#include <ippi.h>
#include <ipps.h>


QVisualizationTab::QVisualizationTab(bool is_streaming, QWidget *parent) :
    QDialog(parent), m_pStreamTab(nullptr), m_pResultTab(nullptr),
    m_pImgObjIntensity(nullptr), m_pImgObjLifetime(nullptr), m_pImgObjMerged(nullptr), 
	m_pImgObjLive(nullptr), m_pImgObjDpcTb(nullptr), m_pImgObjDpcLr(nullptr), m_pImgObjPhase(nullptr),
	m_pMedfilt(nullptr)
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

	for (int i = 0; i < 4; i++)
	{
		ColorTable::colortable ctable;
		QString title;
		switch (i)
		{
		case 0:
			ctable = ColorTable::colortable::gray;
			title = "Brightfield";
			break;
		case 1:
			ctable = ColorTable::colortable(m_pConfig->dpcColorTable);
			title = "Differential Phase Contrast (Vertical; Top-Bottom)";
			break;
		case 2:
			ctable = ColorTable::colortable(m_pConfig->dpcColorTable);
			title = "Differential Phase Contrast (Horizontal; Left-Right)";
			break;
		case 3:
			ctable = ColorTable::colortable(m_pConfig->phaseColorTable);
			title = "Quantitative Phase Imaging";
			break;
		}

		m_pImageView_Dpc[i] = new QImageView(ctable, CMOS_WIDTH, CMOS_HEIGHT);
		m_pImageView_Dpc[i]->setFixedSize(622, 622);
		m_pImageView_Dpc[i]->setSquare(true);
		m_pImageView_Dpc[i]->setVisible(false);
		m_pImageView_Dpc[i]->setEnterCallback([&, i, title]() { m_pImageView_Dpc[i]->setText(QPoint(8, 4), title, false, Qt::green); }); 
		m_pImageView_Dpc[i]->setLeaveCallback([&, i]() { m_pImageView_Dpc[i]->setText(QPoint(8, 4), "", false, Qt::green); });
		m_pImageView_Dpc[i]->setMovedMouseCallback([&](QPoint& p) { m_pStreamTab->getMainWnd()->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
		m_pImageView_Dpc[i]->setClickedMouseCallback([&](int x, int y) {
			m_pImageView_Dpc[i]->getRender()->m_pixelPos[0] = x;
			m_pImageView_Dpc[i]->getRender()->m_pixelPos[1] = y;
			m_pImageView_Dpc[i]->getRender()->update();
		});
	}

    // Create FLIM visualization option tab
    createFlimVisualizationOptionTab();
	createDpcVisualizationOptionTab();

    // Set layout
    m_pGroupBox_VisualizationWidgets = new QGroupBox;
    m_pGroupBox_VisualizationWidgets->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pGroupBox_VisualizationWidgets->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");

    m_pVBoxLayout_VisWidgets = new QVBoxLayout;
	m_pVBoxLayout_VisWidgets->setSpacing(1);

	m_pVBoxLayout_VisWidgets->addWidget(m_pGroupBox_FlimVisualization);
	m_pVBoxLayout_VisWidgets->addWidget(m_pGroupBox_DpcImageMode);

    m_pGroupBox_VisualizationWidgets->setLayout(m_pVBoxLayout_VisWidgets);

	QGridLayout *pGridLayout_ImageView = new QGridLayout;
	pGridLayout_ImageView->setSpacing(1);

	pGridLayout_ImageView->addWidget(m_pImageView_Image, 0, 0, 2, 1);
	pGridLayout_ImageView->addWidget(m_pImageView_Dpc[0], 0, 1);
	pGridLayout_ImageView->addWidget(m_pImageView_Dpc[2], 1, 2);
	pGridLayout_ImageView->addWidget(m_pImageView_Dpc[1], 0, 2);
	pGridLayout_ImageView->addWidget(m_pImageView_Dpc[3], 1, 1);

    m_pVBoxLayout->addItem(pGridLayout_ImageView);

    setLayout(m_pVBoxLayout);

    // Connect signal and slot
    connect(this, SIGNAL(drawImage(bool)), this, SLOT(visualizeImage(bool)));
	connect(this, SIGNAL(plotImage(uint8_t*)), m_pImageView_Image, SLOT(drawImage(uint8_t*)));

	connect(this, SIGNAL(plotLiveImage(uint8_t*)), m_pImageView_Dpc[0], SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(plotDpcTbImage(uint8_t*)), m_pImageView_Dpc[1], SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(plotDpcLrImage(uint8_t*)), m_pImageView_Dpc[2], SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(plotPhaseImage(uint8_t*)), m_pImageView_Dpc[3], SLOT(drawImage(uint8_t*)));
}

QVisualizationTab::~QVisualizationTab()
{
    if (m_pImgObjIntensity) delete m_pImgObjIntensity;
    if (m_pImgObjLifetime) delete m_pImgObjLifetime;
	if (m_pImgObjMerged) delete m_pImgObjMerged;

	if (m_pImgObjLive) delete m_pImgObjLive;
	if (m_pImgObjDpcTb) delete m_pImgObjDpcTb;
	if (m_pImgObjDpcLr) delete m_pImgObjDpcLr;
	if (m_pImgObjPhase) delete m_pImgObjPhase;
	
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
    connect(m_pButtonGroup_ImageMode, SIGNAL(buttonClicked(int)), this, SLOT(changeFlimImageMode(int)));
    connect(m_pComboBox_EmissionChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeEmissionChannel(int)));
    connect(m_pComboBox_LifetimeColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLifetimeColorTable(int)));
    connect(m_pLineEdit_IntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
    connect(m_pLineEdit_IntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
    connect(m_pLineEdit_LifetimeMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
    connect(m_pLineEdit_LifetimeMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustFlimContrast()));
}

void QVisualizationTab::createDpcVisualizationOptionTab()
{
	// Create widgets for DPC visualization option tab
	m_pGroupBox_DpcImageMode = new QGroupBox;
	m_pGroupBox_DpcImageMode->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_pGroupBox_DpcImageMode->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_DpcImageMode->setFixedWidth(312);
	m_pGroupBox_DpcImageMode->setVisible(false);
	QGridLayout *pGridLayout_DpcImageMode = new QGridLayout;
	pGridLayout_DpcImageMode->setSpacing(3);

	m_pGroupBox_DpcVisualization = new QGroupBox;
	m_pGroupBox_DpcVisualization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_pGroupBox_DpcVisualization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_DpcVisualization->setVisible(false);
	m_pGroupBox_DpcVisualization->setFixedWidth(312);
	QGridLayout *pGridLayout_DpcVisualization = new QGridLayout;
	pGridLayout_DpcVisualization->setSpacing(3);

	m_pGroupBox_QpiRegularization = new QGroupBox;
	m_pGroupBox_QpiRegularization->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_pGroupBox_QpiRegularization->setStyleSheet("QGroupBox{padding-top:15px; margin-top:-15px}");
	m_pGroupBox_QpiRegularization->setVisible(false);
	m_pGroupBox_QpiRegularization->setFixedWidth(312);
	QGridLayout *pGridLayout_QpiRegularization = new QGridLayout;
	pGridLayout_QpiRegularization->setSpacing(3);

	// Create widgets for image mode
	m_pLabel_ImageModeDpc = new QLabel(this);
	m_pLabel_ImageModeDpc->setText(" Image Mode   ");

	m_pRadioButton_Live = new QRadioButton(this);
	m_pRadioButton_Live->setText("Live  ");
	m_pRadioButton_Processed = new QRadioButton(this);
	m_pRadioButton_Processed->setText("Processed");

	m_pButtonGroup_ImageModeDpc = new QButtonGroup(this);
	m_pButtonGroup_ImageModeDpc->addButton(m_pRadioButton_Live, DPC_LIVE);
	m_pButtonGroup_ImageModeDpc->addButton(m_pRadioButton_Processed, DPC_PROCESSED);
	m_pRadioButton_Live->setChecked(true);

	// Create widgets for DPC colortable selection
	ColorTable temp_ctable;
	m_pComboBox_DpcColorTable = new QComboBox(this);
	for (int i = 0; i < temp_ctable.m_cNameVector.size(); i++)
		m_pComboBox_DpcColorTable->addItem(temp_ctable.m_cNameVector.at(i));
	m_pComboBox_DpcColorTable->setCurrentIndex(m_pConfig->dpcColorTable);
	m_pLabel_DpcColorTable = new QLabel(" DPC Ctable  ", this);
	m_pLabel_DpcColorTable->setBuddy(m_pComboBox_DpcColorTable);

	// Create widgets for DPC colortable selection
	m_pComboBox_PhaseColorTable = new QComboBox(this);
	for (int i = 0; i < temp_ctable.m_cNameVector.size(); i++)
		m_pComboBox_PhaseColorTable->addItem(temp_ctable.m_cNameVector.at(i));
	m_pComboBox_PhaseColorTable->setCurrentIndex(m_pConfig->phaseColorTable);
	m_pLabel_PhaseColorTable = new QLabel("     Phase Ctable  ", this);
	m_pLabel_PhaseColorTable->setBuddy(m_pComboBox_PhaseColorTable);

	// Create line edit widgets for FLIM contrast adjustment
	m_pLineEdit_LiveIntensityMax = new QLineEdit(this);
	m_pLineEdit_LiveIntensityMax->setFixedWidth(38);
	m_pLineEdit_LiveIntensityMax->setText(QString::number(m_pConfig->liveIntensityRange.max));
	m_pLineEdit_LiveIntensityMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_LiveIntensityMin = new QLineEdit(this);
	m_pLineEdit_LiveIntensityMin->setFixedWidth(38);
	m_pLineEdit_LiveIntensityMin->setText(QString::number(m_pConfig->liveIntensityRange.min));
	m_pLineEdit_LiveIntensityMin->setAlignment(Qt::AlignCenter);
	m_pLineEdit_DpcMax = new QLineEdit(this);
	m_pLineEdit_DpcMax->setFixedWidth(38);
	m_pLineEdit_DpcMax->setText(QString::number(m_pConfig->dpcRange.max, 'f', 2));
	m_pLineEdit_DpcMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_DpcMin = new QLineEdit(this);
	m_pLineEdit_DpcMin->setFixedWidth(38);
	m_pLineEdit_DpcMin->setText(QString::number(m_pConfig->dpcRange.min, 'f', 2));
	m_pLineEdit_DpcMin->setAlignment(Qt::AlignCenter);
	m_pLineEdit_DpcMin->setDisabled(true);
	m_pLineEdit_PhaseMax = new QLineEdit(this);
	m_pLineEdit_PhaseMax->setFixedWidth(38);
	m_pLineEdit_PhaseMax->setText(QString::number(m_pConfig->phaseRange.max, 'f', 2));
	m_pLineEdit_PhaseMax->setAlignment(Qt::AlignCenter);
	m_pLineEdit_PhaseMin = new QLineEdit(this);
	m_pLineEdit_PhaseMin->setFixedWidth(38);
	m_pLineEdit_PhaseMin->setText(QString::number(m_pConfig->phaseRange.min, 'f', 2));
	m_pLineEdit_PhaseMin->setAlignment(Qt::AlignCenter);

	// Create color bar for DPC visualization
	uint8_t color[256];
	for (int i = 0; i < 256; i++)
		color[i] = i;

	m_pImageView_LiveIntensityColorbar = new QImageView(ColorTable::colortable::gray, 256, 1);
	m_pImageView_LiveIntensityColorbar->setFixedSize(140, 15);
	m_pImageView_LiveIntensityColorbar->drawImage(color);
	m_pImageView_DpcColorbar = new QImageView(ColorTable::colortable(m_pConfig->dpcColorTable), 256, 1);
	m_pImageView_DpcColorbar->setFixedSize(140, 15);
	m_pImageView_DpcColorbar->drawImage(color);
	m_pImageView_PhaseColorbar = new QImageView(ColorTable::colortable(m_pConfig->phaseColorTable), 256, 1);
	m_pImageView_PhaseColorbar->setFixedSize(140, 15);
	m_pImageView_PhaseColorbar->drawImage(color);
	m_pLabel_LiveIntensity = new QLabel("CMOS Live  ", this);
	m_pLabel_LiveIntensity->setFixedWidth(70);
	m_pLabel_LiveIntensity->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_pLabel_Dpc = new QLabel("DPC (AU) ", this);
	m_pLabel_Dpc->setFixedWidth(70);
	m_pLabel_Dpc->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
	m_pLabel_Phase = new QLabel("Phase (rad) ", this);
	m_pLabel_Phase->setFixedWidth(70);
	m_pLabel_Phase->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

	// Create QPI regularization widgets
	m_pLabel_L2Reg = new QLabel("L2 Reg  ", this);
	//m_pLabel_L2Reg->setFixedWidth(60);
	
	m_pLineEdit_L2RegAmp = new QLineEdit(this);
	m_pLineEdit_L2RegAmp->setFixedWidth(38);
	m_pLineEdit_L2RegAmp->setText(QString::number(m_pConfig->regL2amp, 'f', 2));
	m_pLineEdit_L2RegAmp->setAlignment(Qt::AlignCenter);

	m_pLineEdit_L2RegPhase = new QLineEdit(this);
	m_pLineEdit_L2RegPhase->setFixedWidth(50);
	m_pLineEdit_L2RegPhase->setText(QString::number(m_pConfig->regL2phase, 'f', 4));
	m_pLineEdit_L2RegPhase->setAlignment(Qt::AlignCenter);

	m_pLabel_TvReg = new QLabel("    TV Reg  ", this);
	//m_pLabel_TvReg->setFixedWidth(60);
	
	m_pLineEdit_TvReg = new QLineEdit(this);
	m_pLineEdit_TvReg->setFixedWidth(38);
	m_pLineEdit_TvReg->setText(QString::number(m_pConfig->regTv, 'f', 2));
	m_pLineEdit_TvReg->setAlignment(Qt::AlignCenter);
	
	// Set layout
	QHBoxLayout *pHBoxLayout_DpcImageMode = new QHBoxLayout;
	pHBoxLayout_DpcImageMode->addWidget(m_pLabel_ImageModeDpc);
	pHBoxLayout_DpcImageMode->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_DpcImageMode->addWidget(m_pRadioButton_Live);
	pHBoxLayout_DpcImageMode->addWidget(m_pRadioButton_Processed);

	//pGridLayout_DpcImageMode->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed), 0, 0);
	pGridLayout_DpcImageMode->addItem(pHBoxLayout_DpcImageMode, 0, 1);

	QHBoxLayout *pHBoxLayout_DpcVisualization1 = new QHBoxLayout;
	pHBoxLayout_DpcVisualization1->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_DpcVisualization1->addWidget(m_pLabel_DpcColorTable);
	pHBoxLayout_DpcVisualization1->addWidget(m_pComboBox_DpcColorTable);
	pHBoxLayout_DpcVisualization1->addWidget(m_pLabel_PhaseColorTable);
	pHBoxLayout_DpcVisualization1->addWidget(m_pComboBox_PhaseColorTable);

	QHBoxLayout *pHBoxLayout_LiveIntensityColorbar = new QHBoxLayout;
	pHBoxLayout_LiveIntensityColorbar->setSpacing(3);
	pHBoxLayout_LiveIntensityColorbar->addWidget(m_pLabel_LiveIntensity);
	pHBoxLayout_LiveIntensityColorbar->addWidget(m_pLineEdit_LiveIntensityMin);
	pHBoxLayout_LiveIntensityColorbar->addWidget(m_pImageView_LiveIntensityColorbar);
	pHBoxLayout_LiveIntensityColorbar->addWidget(m_pLineEdit_LiveIntensityMax);

	QHBoxLayout *pHBoxLayout_DpcColorbar = new QHBoxLayout;
	pHBoxLayout_DpcColorbar->setSpacing(3);
	pHBoxLayout_DpcColorbar->addWidget(m_pLabel_Dpc);
	pHBoxLayout_DpcColorbar->addWidget(m_pLineEdit_DpcMin);
	pHBoxLayout_DpcColorbar->addWidget(m_pImageView_DpcColorbar);
	pHBoxLayout_DpcColorbar->addWidget(m_pLineEdit_DpcMax);

	QHBoxLayout *pHBoxLayout_PhaseColorbar = new QHBoxLayout;
	pHBoxLayout_PhaseColorbar->setSpacing(3);
	pHBoxLayout_PhaseColorbar->addWidget(m_pLabel_Phase);
	pHBoxLayout_PhaseColorbar->addWidget(m_pLineEdit_PhaseMin);
	pHBoxLayout_PhaseColorbar->addWidget(m_pImageView_PhaseColorbar);
	pHBoxLayout_PhaseColorbar->addWidget(m_pLineEdit_PhaseMax);

	pGridLayout_DpcVisualization->addItem(pHBoxLayout_DpcVisualization1, 0, 0);
	pGridLayout_DpcVisualization->addItem(pHBoxLayout_LiveIntensityColorbar, 1, 0);
	pGridLayout_DpcVisualization->addItem(pHBoxLayout_DpcColorbar, 2, 0);
	pGridLayout_DpcVisualization->addItem(pHBoxLayout_PhaseColorbar, 3, 0);
	
	QHBoxLayout *pHBoxLayout_QpiRegularization = new QHBoxLayout;
	pHBoxLayout_QpiRegularization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_QpiRegularization->addWidget(m_pLabel_L2Reg);
	pHBoxLayout_QpiRegularization->addWidget(m_pLineEdit_L2RegAmp);
	pHBoxLayout_QpiRegularization->addWidget(m_pLineEdit_L2RegPhase);
	//pHBoxLayout_QpiRegularization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_QpiRegularization->addWidget(m_pLabel_TvReg);
	pHBoxLayout_QpiRegularization->addWidget(m_pLineEdit_TvReg);

	pGridLayout_QpiRegularization->addItem(pHBoxLayout_QpiRegularization, 0, 0);

	m_pGroupBox_DpcImageMode->setLayout(pGridLayout_DpcImageMode);
	m_pGroupBox_DpcVisualization->setLayout(pGridLayout_DpcVisualization);
	m_pGroupBox_QpiRegularization->setLayout(pGridLayout_QpiRegularization);

	// Connect signal and slot
	connect(m_pButtonGroup_ImageModeDpc, SIGNAL(buttonClicked(int)), this, SLOT(changeDpcImageMode(int)));
	connect(m_pComboBox_DpcColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDpcColorTable(int)));
	connect(m_pComboBox_PhaseColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changePhaseColorTable(int)));
	connect(m_pLineEdit_LiveIntensityMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDpcContrast()));
	connect(m_pLineEdit_LiveIntensityMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDpcContrast()));
	connect(m_pLineEdit_DpcMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDpcContrast()));
	connect(m_pLineEdit_DpcMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDpcContrast()));
	connect(m_pLineEdit_PhaseMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDpcContrast()));
	connect(m_pLineEdit_PhaseMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustDpcContrast()));
	connect(m_pLineEdit_L2RegAmp, SIGNAL(textEdited(const QString &)), this, SLOT(adjustQpiRegParameters()));
	connect(m_pLineEdit_L2RegPhase, SIGNAL(textEdited(const QString &)), this, SLOT(adjustQpiRegParameters()));
	connect(m_pLineEdit_TvReg, SIGNAL(textEdited(const QString &)), this, SLOT(adjustQpiRegParameters()));

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


void QVisualizationTab::setObjects(int n_lines, bool modality)
{
	if (modality)
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
		for (int i = 0; i < 3; i++)
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
	else
	{
		int id = m_pButtonGroup_ImageModeDpc->checkedId();

		// Clear existed buffers
		std::vector<np::FloatArray2> clear_vector1;
		clear_vector1.swap(m_vecIllumImages);

		// Reset image view size
		if (id == DPC_LIVE)
		{
			m_pImageView_Image->resetColormap(ColorTable::gray);
			m_pImageView_Image->resetSize(CMOS_WIDTH, CMOS_HEIGHT);
		}
		else if (id == DPC_PROCESSED)
		{

		}

		// Create visualization buffers
		//if (id == DPC_LIVE)
		//{
			m_liveIntensity = np::FloatArray2(CMOS_WIDTH, CMOS_HEIGHT);
		//}
		//else if (id == DPC_PROCESSED)
		//{
			if (m_vecIllumImages.size() == 0)
			{
				for (int i = 0; i < 4; i++)
				{
					np::FloatArray2 illum = np::FloatArray2(CMOS_WIDTH, CMOS_HEIGHT);
					m_vecIllumImages.push_back(illum);
				}
			}
		//}

		// Create image visualization buffers
		ColorTable temp_ctable;
		if (m_pImgObjLive) delete m_pImgObjLive;
		m_pImgObjLive = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(ColorTable::gray));
		if (m_pImgObjDpcTb) delete m_pImgObjDpcTb;
		m_pImgObjDpcTb = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(m_pConfig->dpcColorTable));
		if (m_pImgObjDpcLr) delete m_pImgObjDpcLr;
		m_pImgObjDpcLr = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(m_pConfig->dpcColorTable));
		if (m_pImgObjPhase) delete m_pImgObjPhase;
		m_pImgObjPhase = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(m_pConfig->phaseColorTable));
	}
}


void QVisualizationTab::visualizeImage(bool modality)
{
	if (modality)
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
	else
	{
		int id = m_pButtonGroup_ImageModeDpc->checkedId();

		if (id == DPC_LIVE)
		{
			// Live CMOS image
			ippiScale_32f8u_C1R(m_liveIntensity, sizeof(float) * CMOS_WIDTH, m_pImgObjLive->arr.raw_ptr(), sizeof(uint8_t) * CMOS_WIDTH,
				{ CMOS_WIDTH, CMOS_HEIGHT }, (Ipp32f)m_pConfig->liveIntensityRange.min, (Ipp32f)m_pConfig->liveIntensityRange.max);

			emit plotImage(m_pImgObjLive->qindeximg.bits());
		}
		else if (id == DPC_PROCESSED)
		{
			QpiProcess* pQpi = m_pStreamTab->getOperationTab()->getDataAcq()->getQpi();			
			// 0 top 1 left 2 bottom 3 right
			
			// Brightfield image
			pQpi->getBrightfield(m_vecIllumImages);
			
			// DPC top-bottom
			pQpi->getDpc(m_vecIllumImages, top_bottom);

			// DPC left-right
			pQpi->getDpc(m_vecIllumImages, left_right);

			// DPC fusion
			pQpi->getQpi(m_vecIllumImages);

			// Scaling
			ippiScale_32f8u_C1R(pQpi->brightfield, sizeof(float) * CMOS_WIDTH, m_pImgObjLive->arr.raw_ptr(), sizeof(uint8_t) * CMOS_WIDTH,
				{ CMOS_WIDTH, CMOS_HEIGHT }, (Ipp32f)m_pConfig->liveIntensityRange.min, (Ipp32f)m_pConfig->liveIntensityRange.max);
			ippiScale_32f8u_C1R(pQpi->dpc_tb, sizeof(float) * CMOS_WIDTH, m_pImgObjDpcTb->arr.raw_ptr(), sizeof(uint8_t) * CMOS_WIDTH,
				{ CMOS_WIDTH, CMOS_HEIGHT }, (Ipp32f)m_pConfig->dpcRange.min, (Ipp32f)m_pConfig->dpcRange.max);
			ippiScale_32f8u_C1R(pQpi->dpc_lr, sizeof(float) * CMOS_WIDTH, m_pImgObjDpcLr->arr.raw_ptr(), sizeof(uint8_t) * CMOS_WIDTH,
				{ CMOS_WIDTH, CMOS_HEIGHT }, (Ipp32f)m_pConfig->dpcRange.min, (Ipp32f)m_pConfig->dpcRange.max);
			ippiScale_32f8u_C1R(pQpi->phase, sizeof(float) * CMOS_WIDTH, m_pImgObjPhase->arr.raw_ptr(), sizeof(uint8_t) * CMOS_WIDTH,
				{ CMOS_WIDTH, CMOS_HEIGHT }, (Ipp32f)m_pConfig->phaseRange.min, (Ipp32f)m_pConfig->phaseRange.max);

			// Signaling
			emit plotLiveImage(m_pImgObjLive->qindeximg.bits());
			emit plotDpcTbImage(m_pImgObjDpcTb->qindeximg.bits());
			emit plotDpcLrImage(m_pImgObjDpcLr->qindeximg.bits());
			emit plotPhaseImage(m_pImgObjPhase->qindeximg.bits());
		}
	}
}


void QVisualizationTab::changeFlimImageMode(int mode)
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

    visualizeImage(m_pStreamTab->getCurrentModality());
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

    visualizeImage(m_pStreamTab->getCurrentModality());
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

    visualizeImage(m_pStreamTab->getCurrentModality());
}

void QVisualizationTab::adjustFlimContrast()
{
    m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_IntensityMin->text().toFloat();
    m_pConfig->flimIntensityRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_IntensityMax->text().toFloat();
    m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].min = m_pLineEdit_LifetimeMin->text().toFloat();
    m_pConfig->flimLifetimeRange[m_pConfig->flimEmissionChannel - 1].max = m_pLineEdit_LifetimeMax->text().toFloat();

    visualizeImage(m_pStreamTab->getCurrentModality());
}

void QVisualizationTab::changeDpcImageMode(int mode)
{
	// Set alternative illumination callback object
	ImagingSource *pImagingSource = m_pStreamTab->getOperationTab()->getDataAcq()->getImagingSource();
	{
		std::unique_lock<std::mutex> mlock(pImagingSource->mutex_);

		pImagingSource->SetIllumPattern.clear();
		if (mode == DPC_LIVE)
		{
			pImagingSource->SetIllumPattern += [&](int frame_count)
			{
				(void)frame_count;
			};
		}
		else if (mode == DPC_PROCESSED)
		{
			pImagingSource->SetIllumPattern += [&](int frame_count)
			{
				switch (frame_count % 4)
				{
				case 0:
					m_pStreamTab->getDeviceControlTab()->getRadioButtonTop()->setChecked(true);
					m_pStreamTab->getDeviceControlTab()->setDpcIllumPattern(illum_top);
					break;
				case 1:
					m_pStreamTab->getDeviceControlTab()->getRadioButtonLeft()->setChecked(true);
					m_pStreamTab->getDeviceControlTab()->setDpcIllumPattern(illum_bottom);
					break;
				case 2:
					m_pStreamTab->getDeviceControlTab()->getRadioButtonBottom()->setChecked(true);
					m_pStreamTab->getDeviceControlTab()->setDpcIllumPattern(illum_left);
					break;
				case 3:
					m_pStreamTab->getDeviceControlTab()->getRadioButtonRight()->setChecked(true);
					m_pStreamTab->getDeviceControlTab()->setDpcIllumPattern(illum_right);
					break;
				}
			};
		}
	}

	// Set image view
	if (mode == DPC_LIVE)
	{
		m_pStreamTab->getDeviceControlTab()->getDpcIlluminationGroupBox()->setEnabled(true);
		m_pImageView_Image->setVisible(true);
		for (int i = 0; i < 4; i++)
			m_pImageView_Dpc[i]->setVisible(false);
	}
	else if (mode == DPC_PROCESSED)
	{
		m_pStreamTab->getDeviceControlTab()->getDpcIlluminationGroupBox()->setEnabled(false);
		m_pImageView_Image->setVisible(false);
		for (int i = 0; i < 4; i++)
			m_pImageView_Dpc[i]->setVisible(true);		
	}
}

void QVisualizationTab::changeDpcColorTable(int ctable_ind)
{
	m_pConfig->dpcColorTable = ctable_ind;

	if (m_pButtonGroup_ImageModeDpc->checkedId() == DPC_PROCESSED)
	{
		m_pImageView_Dpc[1]->resetColormap(ColorTable::colortable(ctable_ind));		
		m_pImageView_Dpc[2]->resetColormap(ColorTable::colortable(ctable_ind));
	}
	m_pImageView_DpcColorbar->resetColormap(ColorTable::colortable(ctable_ind));
	
	ColorTable temp_ctable;
	if (m_pImgObjDpcTb) delete m_pImgObjDpcTb;
	m_pImgObjDpcTb = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(m_pConfig->dpcColorTable));
	if (m_pImgObjDpcLr) delete m_pImgObjDpcLr;
	m_pImgObjDpcLr = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(m_pConfig->dpcColorTable));

	visualizeImage(m_pStreamTab->getCurrentModality());
}

void QVisualizationTab::changePhaseColorTable(int ctable_ind)
{
	m_pConfig->phaseColorTable = ctable_ind;

	if (m_pButtonGroup_ImageModeDpc->checkedId() == DPC_PROCESSED)
		m_pImageView_Dpc[3]->resetColormap(ColorTable::colortable(ctable_ind));
	m_pImageView_PhaseColorbar->resetColormap(ColorTable::colortable(ctable_ind));

	ColorTable temp_ctable;
	if (m_pImgObjPhase) delete m_pImgObjPhase;
	m_pImgObjPhase = new ImageObject(CMOS_WIDTH, CMOS_HEIGHT, temp_ctable.m_colorTableVector.at(m_pConfig->phaseColorTable));

	visualizeImage(m_pStreamTab->getCurrentModality());
}

void QVisualizationTab::adjustDpcContrast()
{
	m_pConfig->liveIntensityRange.min = m_pLineEdit_LiveIntensityMin->text().toUShort();
	m_pConfig->liveIntensityRange.max = m_pLineEdit_LiveIntensityMax->text().toUShort();
	m_pConfig->dpcRange.min = -m_pLineEdit_DpcMax->text().toFloat();
	m_pConfig->dpcRange.max = m_pLineEdit_DpcMax->text().toFloat();
	m_pLineEdit_DpcMin->setText(QString::number(m_pConfig->dpcRange.min, 'f', 2));
	m_pConfig->phaseRange.min = m_pLineEdit_PhaseMin->text().toFloat();
	m_pConfig->phaseRange.max = m_pLineEdit_PhaseMax->text().toFloat();
	
	visualizeImage(m_pStreamTab->getCurrentModality());
}

void QVisualizationTab::adjustQpiRegParameters()
{
	m_pConfig->regL2amp = m_pLineEdit_L2RegAmp->text().toFloat();
	m_pConfig->regL2phase = m_pLineEdit_L2RegPhase->text().toFloat();
	m_pConfig->regTv = m_pLineEdit_TvReg->text().toFloat();

	QpiProcess* pQpi = m_pStreamTab->getOperationTab()->getDataAcq()->getQpi();
	pQpi->init_qpi_buffs(PUPIL_RADIUS, m_pConfig->regL2amp, m_pConfig->regL2phase, m_pConfig->regTv);
}