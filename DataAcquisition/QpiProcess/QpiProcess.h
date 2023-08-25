#ifndef QPI_PROCESS_H
#define QPI_PROCESS_H

#include <Doulos/Configuration.h>

#include <iostream>
#include <vector>
#include <utility>
#include <cmath>

#include <ipps.h>
#include <ippi.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <mkl_df.h>

#include <Common/array.h>
#include <Common/callback.h>
using namespace np;

enum compo_orientation
{
	top, left, bottom, right
};

enum dpc_orientation
{
	top_bottom, left_right, fusion
};


struct FFT2_R2C
{
public:
	FFT2_R2C() :
		pFFTSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr)
	{
	}

	~FFT2_R2C()
	{
		if (pFFTSpec) { ippsFree(pFFTSpec); pFFTSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void operator() (Ipp32fc* dst, const Ipp32f* src, int num = 0)
	{
		np::FloatArray2 packed_coeffs(width, height);
		ippiFFTFwd_RToPack_32f_C1R(src, sizeof(float) * width, packed_coeffs, sizeof(float) * width, pFFTSpec, pMemBuffer + num * sizeBuffer);
		ippiPackToCplxExtend_32f32fc_C1R(packed_coeffs, { width, height }, sizeof(float) * width, dst, sizeof(float) * 2 * width);
	}

	void initialize(int _width, int _height)
	{
		// init FFT spec
		width = _width;
		height = _height;

		const int ORDERx = (int)(ceil(log2(_width)));
		const int ORDERy = (int)(ceil(log2(_height)));

		int sizeSpec, sizeInit; 
		ippiFFTGetSize_R_32f(ORDERx, ORDERy, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

		pFFTSpec = (IppiFFTSpec_R_32f*)ippsMalloc_8u(sizeSpec);
		pMemInit = (Ipp8u*)ippsMalloc_8u(sizeInit);
		pMemBuffer = (Ipp8u*)ippsMalloc_8u(4 * sizeBuffer);

		ippiFFTInit_R_32f(ORDERx, ORDERy, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pFFTSpec, pMemInit);
	}

private:
	int width, height;
	IppiFFTSpec_R_32f* pFFTSpec;	
	Ipp8u* pMemInit;
	Ipp8u* pMemBuffer;
	int sizeBuffer;
};

struct FFT2_C2C
{
	FFT2_C2C() :
		pFFTSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr)
	{
	}

	~FFT2_C2C()
	{
		if (pFFTSpec) { ippsFree(pFFTSpec); pFFTSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void forward(Ipp32fc* dst, const Ipp32fc* src, int num = 0)
	{
		ippiFFTFwd_CToC_32fc_C1R(src, sizeof(Ipp32fc) * width, dst, sizeof(Ipp32fc) * width, pFFTSpec, pMemBuffer + num * sizeBuffer);
	}

	void inverse(Ipp32fc* dst, const Ipp32fc* src, int num = 0)
	{
		ippiFFTInv_CToC_32fc_C1R(src, sizeof(Ipp32fc) * width, dst, sizeof(Ipp32fc) * width, pFFTSpec, pMemBuffer + num * sizeBuffer);
	}

	void initialize(int _width, int _height)
	{
		// init FFT spec
		width = _width;
		height = _height;

		const int ORDERx = (int)(ceil(log2(_width)));
		const int ORDERy = (int)(ceil(log2(_height)));

		int sizeSpec, sizeInit;
		ippiFFTGetSize_C_32fc(ORDERx, ORDERy, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

		pFFTSpec = (IppiFFTSpec_C_32fc*)ippsMalloc_8u(sizeSpec);
		pMemInit = (Ipp8u*)ippsMalloc_8u(sizeInit);
		pMemBuffer = (Ipp8u*)ippsMalloc_8u(4 * sizeBuffer);

		ippiFFTInit_C_32fc(ORDERx, ORDERy, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pFFTSpec, pMemInit);
	}

private:
	int width, height;
	IppiFFTSpec_C_32fc* pFFTSpec;
	Ipp8u* pMemInit;
	Ipp8u* pMemBuffer;
	int sizeBuffer;
};


class QpiProcess
{
// Methods
public: // Constructor & Destructor
	explicit QpiProcess(int _width, int _height, float pupil_radius, float _reg_l2_a, float _reg_l2_ph, float _reg_tv);
	~QpiProcess();

private: // Not to call copy constrcutor and copy assignment operator
	QpiProcess(const QpiProcess&);
	QpiProcess& operator=(const QpiProcess&);

public:
	// Initialize QPI recon buffers
	void init_qpi_buffs(float pupil_radius, float _reg_l2_a, float _reg_l2_ph, float _reg_tv);
	
	// Get processed images
	void getBrightfield(std::vector<np::FloatArray2>& _compo);
	void getDpc(std::vector<np::FloatArray2>& _compo, dpc_orientation orientation);
	void getQpi(std::vector<np::FloatArray2>& _compo);
	
// Variables
private:	
	// Basic parameters
	int width, height;
	float reg_l2_a, reg_l2_ph, reg_tv;
	
	// 2D Fourier transform objects
	FFT2_R2C fft2r_pd, fft2r;
	FFT2_C2C fft2c_pd, fft2c;

	// DPC recon buffers
	np::FloatArray2 _sub, _add;

	// QPI recon buffers	
	std::vector<np::ComplexFloatArray2> ph_H, ph_Hc;
	np::FloatArray2 tv_term;
	np::FloatArray2 M11, M22, denom;	
	
public:
	// Image buffers	
	np::FloatArray2 brightfield;
	np::FloatArray2 dpc_tb, dpc_lr;
	np::FloatArray2 phase;

public:
	// Callbacks
	callback<const char*> SendStatusMessage;
};

#endif
