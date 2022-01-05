
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <Doulos/Configuration.h>

#include <Doulos/QStreamTab.h>
#include <Doulos/QOperationTab.h>
#include <Doulos/QDeviceControlTab.h>

#include <Doulos/Dialog/FlimCalibDlg.h>

#include <DataAcquisition/DataAcquisition.h>
#include <DataAcquisition/FLImProcess/FLImProcess.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize user interface
    QString windowTitle("Doulos [%1] v%2");
    this->setWindowTitle(windowTitle.arg("Organoid FLIM").arg(VERSION));

    // Create configuration object
    m_pConfiguration = new Configuration;
    m_pConfiguration->getConfigFile("Doulos.ini");

	m_pConfiguration->nScans = N_SCANS;
	m_pConfiguration->nTimes = N_TIMES;

	m_pConfiguration->nPixels = N_PIXELS;
	m_pConfiguration->nTimes = N_TIMES;

	m_pConfiguration->flimLaserRepRate = FLIM_LASER_REP_RATE;

	m_pConfiguration->bufferSize = m_pConfiguration->nScans * m_pConfiguration->nTimes;
	m_pConfiguration->imageSize = m_pConfiguration->nPixels * m_pConfiguration->nLines;

    // Set timer for renew configuration
    m_pTimer = new QTimer(this);
    m_pTimer->start(5 * 60 * 1000); // renew per 5 min
	m_pTimerSync = new QTimer(this);
	m_pTimerSync->start(1000); // renew per 1 sec

    // Create tabs objects
    m_pStreamTab = new QStreamTab(this);

    // Create group boxes and tab widgets
    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pStreamTab, tr("Real-Time Data Streaming"));
	
    // Create status bar
    QLabel *pStatusLabel_Temp1 = new QLabel(this); // System Message?
    m_pStatusLabel_ImagePos = new QLabel(QString("(%1, %2)").arg(0000, 4).arg(0000, 4), this);
    	
	size_t fp_bfn = m_pStreamTab->getFlimProcessingBufferQueueSize();
	size_t fv_bfn = m_pStreamTab->getFlimVisualizationBufferQueueSize();
	
	m_pStatusLabel_SyncStatus = new QLabel(QString("FP bufn: %1 / FV bufn: %2 ")
		.arg(fp_bfn, 3).arg(fv_bfn, 3), this);

    pStatusLabel_Temp1->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_pStatusLabel_ImagePos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pStatusLabel_SyncStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);

    // then add the widget to the status bar
    statusBar()->addPermanentWidget(pStatusLabel_Temp1, 6);
    statusBar()->addPermanentWidget(m_pStatusLabel_ImagePos, 1);
    statusBar()->addPermanentWidget(m_pStatusLabel_SyncStatus, 2);

    // Set layout
    m_pGridLayout = new QGridLayout;
    m_pGridLayout->addWidget(m_pTabWidget, 0, 0);

    ui->centralWidget->setLayout(m_pGridLayout);

    this->setFixedSize(1340, 1050);

    // Connect signal and slot
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
	connect(m_pTimerSync, SIGNAL(timeout()), this, SLOT(onTimerSync()));
    connect(m_pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(changedTab(int)));
}

MainWindow::~MainWindow()
{
	m_pTimer->stop();
	m_pTimerSync->stop();

    if (m_pConfiguration)
    {
        m_pConfiguration->setConfigFile("Doulos.ini");
        delete m_pConfiguration;
    }

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_pStreamTab->getOperationTab()->isAcquisitionButtonToggled())
        m_pStreamTab->getOperationTab()->setAcquisitionButton(false);

	m_pStreamTab->getDeviceControlTab()->setControlsStatus(false);

    if (m_pStreamTab->getDeviceControlTab()->getFlimCalibDlg())
        m_pStreamTab->getDeviceControlTab()->getFlimCalibDlg()->close();

    e->accept();
}


void MainWindow::onTimer()
{
    // Current Time should be added here.
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    QString current = QString("%1-%2-%3 %4-%5-%6")
        .arg(date.year()).arg(date.month(), 2, 10, (QChar)'0').arg(date.day(), 2, 10, (QChar)'0')
        .arg(time.hour(), 2, 10, (QChar)'0').arg(time.minute(), 2, 10, (QChar)'0').arg(time.second(), 2, 10, (QChar)'0');

	m_pConfiguration->msgHandle(QString("[%1] Configuration data has been renewed!").arg(current).toLocal8Bit().data());

	m_pStreamTab->getOperationTab()->getDataAcq()->getFLIm()->saveMaskData();
    m_pConfiguration->setConfigFile("Doulos.ini");
}

void MainWindow::onTimerSync()
{
	size_t fp_bfn = m_pStreamTab->getFlimProcessingBufferQueueSize();
	size_t fv_bfn = m_pStreamTab->getFlimVisualizationBufferQueueSize();

	m_pStatusLabel_SyncStatus->setText(QString("FP bufn: %1 / FV bufn: %2 ")
		.arg(fp_bfn, 3).arg(fv_bfn, 3));
}

void MainWindow::changedTab(int index)
{
    (void)index;
}
