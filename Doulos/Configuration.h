#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"1.0.5"

#define POWER_2(x)					(1 << x)
#define NEAR_2_POWER(x)				(int)(1 << (int)ceil(log2(x)))

//////////////////////// Image Setup /////////////////////////
#define ADC_RATE					500 // MegaSamps/sec

#define N_SCANS		                384 //384 //768 // buffer width (should be 4's multiples)
#define N_TIMES						500 // buffer height

#define N_PIXELS					2 * N_TIMES//4 * N_TIMES // image width (will be deprecated)

//////////////////////// FLIM Setup /////////////////////////
#define FLIM_LASER_COM_PORT			"COM3"
#define FLIM_LASER_REP_RATE			1'100'000.0 
#define FLIM_LASER_FINITE_SAMPS		2'000 // * CRS_SCAN_FREQ = REP_RATE

#define ALAZAR_TRIG_DELAY			0  // (will be deprecated)

#define POWER_LEVEL_LINE			"Dev1/port0/line0:7"
#define POWER_LATCH_LINE			"Dev1/port0/line8"
#define EM_ENABLE_LINE				"Dev1/port0/line9:11"
#define MONITORING_LINE				"Dev1/port0/line12:14"

#define NI_FLIM_TRIG_CHANNEL		"Dev1/ctr2"  // PFI14
#define NI_FLIM_TRIG_SOURCE			"/Dev1/PFI5"

#define ALAZAR_DAQ_TRIG_CHANNEL		"Dev1/ctr1"  // PFI13
#define ALAZAR_DAQ_TRIG_SOURCE		"/Dev1/PFI7"

//#define NI_MASTER_TRIG_CHANNEL		"Dev1/ctr3" // PFI15
//#define NI_MASTER_TRIG_SOURCE		"/Dev1/PFI4"

#define NI_PMT_GAIN_CHANNEL		    "Dev1/ao2"

#define AUTO_STOP_AFTER_REC

//////////////////////// Scan Setup /////////////////////////
#define FAST_SCAN_FREQ				500.0
#define FAST_DIR_FACTOR				1 // 1: unidirectional, 2: bidirectional
#define FAST_TOTAL_PIECES			4
#define FAST_VALID_PIECES			2 // full recovery

#define NI_GALVO_CHANNEL			"Dev1/ao0:1"
#define NI_GALVO_SOURCE				NI_FLIM_TRIG_SOURCE
#define NI_GALVO_SOURCE_BIDIR		"/Dev1/ChangeDetectionEvent"
#define NI_GALVO_START_TRIG_SOURCE	"/Dev1/PFI6"
#define NI_GALVO_TWO_EDGE_LINE		"Dev1/port0/line16"
#define GALVO_FLYING_BACK			10  // flying-back pixel (should be even)

#define NANOSCOPE_STAGE_PORT		"COM4"

//////////////// Thread & Buffer Processing /////////////////
#define PROCESSING_BUFFER_SIZE		100
#define WRITING_IMAGE_SIZE          100	

///////////////////// FLIm Processing ///////////////////////
#define FLIM_CH_START_5				50 
#define GAUSSIAN_FILTER_WIDTH		250  // (will be deprecated)
#define GAUSSIAN_FILTER_STD			60  // (will be deprecated)
#define FLIM_SPLINE_FACTOR			10  // (will be deprecated)
#define INTENSITY_THRES				0.01f

//////////// Differential Phase Contrast Imaging ////////////
#define CMOS_WIDTH					2048
#define CMOS_HEIGHT					2048
#define ILLUMINATION_COM_PORT		"COM5"
#define PUPIL_RADIUS				803.3548

/////////////////////// Visualization ///////////////////////
#define INTENSITY_COLORTABLE		19 // viridis


template <typename T>
struct Range
{
	T min = 0;
	T max = 0;
};

enum voltage_range
{
    v0_220 = 1, v0_247, v0_277, v0_311, v0_349,
    v0_391, v0_439, v0_493, v0_553, v0_620,
    v0_696, v0_781, v0_876, v0_983, v1_103,
    v1_237, v1_388, v1_557, v1_748, v1_961,
    v2_200, v2_468, v2_770, v3_108, v3_487
};


#include <QString>
#include <QSettings>
#include <QDateTime>

#include <Common/callback.h>


class Configuration
{
public:
	explicit Configuration() : imageAveragingFrames(1), biDirScanComp(0.0f), flimLaserPower(0), crsCompensation(false), flimEmissionChannel(1) {}
	~Configuration() {}

public:
    void getConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");

        // Image size
		nScans = settings.value("nScans").toInt();		
		nTimes = settings.value("nTimes").toInt();
		nPixels = settings.value("nPixels").toInt();
		nLines = settings.value("nLines").toInt();

		bufferSize = nScans * nTimes;
		imageSize = nPixels * nLines;
				
		// Image averaging
		imageAveragingFrames = settings.value("imageAveragingFrames").toInt();

		// Bidirectional scan compensation
		biDirScanComp = settings.value("biDirScanComp").toFloat();

		//CRS nonlinearity compensation
		crsCompensation = settings.value("crsCompensation").toBool();
		
		// Image stitching
		imageStichingXStep = settings.value("imageStichingXStep").toInt();
		imageStichingYStep = settings.value("imageStichingYStep").toInt();

        // FLIm processing
		flimBg = settings.value("flimBg").toFloat();
		flimWidthFactor = settings.value("flimWidthFactor").toFloat();
		for (int i = 0; i < 4; i++)
		{
			flimChStartInd[i] = settings.value(QString("flimChStartInd_%1").arg(i)).toInt();
			if (i != 0)
				flimDelayOffset[i - 1] = settings.value(QString("flimDelayOffset_%1").arg(i)).toFloat();
        }

        // Visualization
        flimEmissionChannel = settings.value("flimEmissionChannel").toInt();
        flimLifetimeColorTable = settings.value("flimLifetimeColorTable").toInt();
		dpcColorTable = settings.value("dpcColorTable").toInt();
		phaseColorTable = settings.value("phaseColorTable").toInt();
		for (int i = 0; i < 3; i++)
		{
			flimIntensityRange[i].max = settings.value(QString("flimIntensityRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimIntensityRange[i].min = settings.value(QString("flimIntensityRangeMin_Ch%1").arg(i + 1)).toFloat();
			flimLifetimeRange[i].max = settings.value(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1)).toFloat();
			flimLifetimeRange[i].min = settings.value(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1)).toFloat();
		}
		liveIntensityRange.max = settings.value("liveIntensityRangeMax").toInt();
		liveIntensityRange.min = settings.value("liveIntensityRangeMin").toInt();
		dpcRange.max = settings.value("dpcRangeMax").toFloat();
		dpcRange.min = settings.value("dpcRangeMin").toFloat();
		phaseRange.max = settings.value("phaseRangeMax").toFloat();
		phaseRange.min = settings.value("phaseRangeMin").toFloat();
		regL2amp = settings.value("regL2amp").toFloat();
		regL2phase = settings.value("regL2phase").toFloat();
		regTv = settings.value("regTv").toFloat();
		
		//for (int i = 0; i < 3; i++)
		//{
		//	settings.setValue(QString("flimIntensityRangeMax_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].max, 'f', 1));
		//	settings.setValue(QString("flimIntensityRangeMin_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].min, 'f', 1));
		//	settings.setValue(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].max, 'f', 1));
		//	settings.setValue(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].min, 'f', 1));
		//}
		//settings.setValue(QString("liveIntensityRangeMax"), QString::number(liveIntensityRange.max));
		//settings.setValue(QString("liveIntensityRangeMin"), QString::number(liveIntensityRange.min));
		
		// Device control
        pmtGainVoltage = settings.value("pmtGainVoltage").toFloat();
        flimLaserRepRate = settings.value("flimLaserRepRate").toInt();
		flimLaserPower = settings.value("flimLaserPower").toInt();
		resonantScanVoltage = settings.value("resonantScanVoltage").toFloat();
        galvoScanVoltage = settings.value("galvoScanVoltage").toFloat();
        galvoScanVoltageOffset = settings.value("galvoScanVoltageOffset").toFloat();        		
		for (int i = 0; i < 3; i++)
		{
			NanoscopePosition[i] = settings.value(QString("NanoscopePosition_%1").arg(i + 1)).toInt();
			NanoscopeSpeed[i] = settings.value(QString("NanoscopeSpeed_%1").arg(i + 1)).toInt();
			NanoscopeStep[i] = settings.value(QString("NanoscopeSteyp_%1").arg(i + 1)).toInt();
		}
		cmosGain = settings.value("cmosGain").toInt();
		cmosExposure = settings.value("cmosExposure").toInt();

		settings.endGroup();
	}

	void setConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");
		
        // Image size
		settings.setValue("nScans", nScans);
		settings.setValue("nTimes", nTimes);

		settings.setValue("nPixels", nPixels);
		settings.setValue("nLines", nLines);

		settings.setValue("bufferSize", bufferSize);
		settings.setValue("imageSize", imageSize);

		// Image averaging
		settings.setValue("imageAveragingFrames", imageAveragingFrames);
		
		// Bidirectional scan compensation
		settings.setValue("biDirScanComp", QString::number(biDirScanComp, 'f', 2));
		
		// CRS nonlinearity compensation
		settings.setValue("crsCompensation", crsCompensation);

		// Image stitching
		settings.setValue("imageStichingXStep", imageStichingXStep);
		settings.setValue("imageStichingYStep", imageStichingYStep);
		
        // FLIm processing
		settings.setValue("flimBg", QString::number(flimBg, 'f', 2));
		settings.setValue("flimWidthFactor", QString::number(flimWidthFactor, 'f', 2)); 
		for (int i = 0; i < 4; i++)
		{
			settings.setValue(QString("flimChStartInd_%1").arg(i), flimChStartInd[i]);
			if (i != 0)
				settings.setValue(QString("flimDelayOffset_%1").arg(i), QString::number(flimDelayOffset[i - 1], 'f', 3));
		}

        // Visualization
        settings.setValue("flimEmissionChannel", flimEmissionChannel);
		settings.setValue("flimLifetimeColorTable", flimLifetimeColorTable);
		settings.setValue("dpcColorTable", dpcColorTable);
		settings.setValue("phaseColorTable", phaseColorTable);
		for (int i = 0; i < 3; i++)
		{
			settings.setValue(QString("flimIntensityRangeMax_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].max, 'f', 1));
			settings.setValue(QString("flimIntensityRangeMin_Ch%1").arg(i + 1), QString::number(flimIntensityRange[i].min, 'f', 1));
			settings.setValue(QString("flimLifetimeRangeMax_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].max, 'f', 1));
			settings.setValue(QString("flimLifetimeRangeMin_Ch%1").arg(i + 1), QString::number(flimLifetimeRange[i].min, 'f', 1));
		}
		settings.setValue("liveIntensityRangeMax", QString::number(liveIntensityRange.max));
		settings.setValue("liveIntensityRangeMin", QString::number(liveIntensityRange.min));
		settings.setValue("dpcRangeMax", QString::number(dpcRange.max, 'f', 2));
		settings.setValue("dpcRangeMin", QString::number(dpcRange.min, 'f', 2));
		settings.setValue("phaseRangeMax", QString::number(phaseRange.max, 'f', 2));
		settings.setValue("phaseRangeMin", QString::number(phaseRange.min, 'f', 2));
		settings.setValue("regL2amp", QString::number(regL2amp, 'f', 2));
		settings.setValue("regL2phase", QString::number(regL2phase, 'f', 4));
		settings.setValue("regTv", QString::number(regTv, 'f', 2));
		
		// Device control
        settings.setValue("pmtGainVoltage", QString::number(pmtGainVoltage, 'f', 2));
        settings.setValue("flimLaserRepRate", flimLaserRepRate);
		settings.setValue("flimLaserPower", flimLaserPower);
		settings.setValue("resonantScanVoltage", QString::number(resonantScanVoltage, 'f', 2));
        settings.setValue("galvoScanVoltage", QString::number(galvoScanVoltage, 'f', 1));
        settings.setValue("galvoScanVoltageOffset", QString::number(galvoScanVoltageOffset, 'f', 1));    
		for (int i = 0; i < 3; i++)
		{
			settings.setValue(QString("NanoscopePosition_%1").arg(i + 1), NanoscopePosition[i]);
			settings.setValue(QString("NanoscopeSpeed_%1").arg(i + 1), NanoscopeSpeed[i]);
			settings.setValue(QString("NanoscopeSteyp_%1").arg(i + 1), NanoscopeStep[i]);
		}
		settings.setValue("cmosGain", cmosGain);
		settings.setValue("cmosExposure", cmosExposure);

		// Current Time
		QDate date = QDate::currentDate();
		QTime time = QTime::currentTime();
		settings.setValue("time", QString("%1-%2-%3 %4-%5-%6")
			.arg(date.year()).arg(date.month(), 2, 10, (QChar)'0').arg(date.day(), 2, 10, (QChar)'0')
			.arg(time.hour(), 2, 10, (QChar)'0').arg(time.minute(), 2, 10, (QChar)'0').arg(time.second(), 2, 10, (QChar)'0'));

		settings.endGroup();
	}
	
public:
    // Image size parameters
	int nScans, nTimes;		
	int nPixels, nLines;	
	int bufferSize, imageSize;
	
	// Image averaging
	int imageAveragingFrames;

	// Bidirectional scan compensation
	float biDirScanComp;
	
	// CRS nonlinearity compensation
	bool crsCompensation;

	// Image stitching
	int imageStichingXStep;
	int imageStichingYStep;

    // FLIm processing
	float flimBg;
	float flimWidthFactor;
	int flimChStartInd[4];
    float flimDelayOffset[3];

	// Visualization    
    int flimEmissionChannel;
    int flimLifetimeColorTable;
	int dpcColorTable, phaseColorTable;
    Range<float> flimIntensityRange[3];
    Range<float> flimLifetimeRange[3];
	Range<uint16_t> liveIntensityRange;
	Range<float> dpcRange, phaseRange;
	float regL2amp, regL2phase, regTv;

	// Device control
    float pmtGainVoltage;

	int alazarTriggerDelay;

    int flimLaserRepRate;
	int flimLaserPower;

	float resonantScanVoltage;
    float galvoScanVoltage;
    float galvoScanVoltageOffset;    

	int NanoscopePosition[3];
	int NanoscopeSpeed[3];
	int NanoscopeStep[3];

	int cmosGain, cmosExposure;

	// Message callback
	callback<const char*> msgHandle;
};


#endif // CONFIGURATION_H
