
#include "QpiProcess.h"
#include <QFile>

QpiProcess::QpiProcess(int _width, int _height, float pupil_radius, float _reg_l2_a, float _reg_l2_ph, float _reg_tv) :
	width(_width), height(_height), reg_l2_a(_reg_l2_a), reg_l2_ph(_reg_l2_ph), reg_tv(_reg_tv)
{
	// DPC recon buffers
	_sub = np::FloatArray2(width, height);
	_add = np::FloatArray2(width, height);

	// QPI recon buffers	
	for (int i = 0; i < 4; i++)
	{
		np::ComplexFloatArray2 _ph_H(width, height);
		np::ComplexFloatArray2 _ph_Hc(width, height);
		ph_H.push_back(_ph_H);
		ph_Hc.push_back(_ph_Hc);
	}
	tv_term = np::FloatArray2(width, height);
	M11 = np::FloatArray2(width, height);
	M22 = np::FloatArray2(width, height);
	denom = np::FloatArray2(width, height);
		
	// Image buffers
	brightfield = np::FloatArray2(width, height);;
	dpc_tb = np::FloatArray2(width, height); 
	dpc_lr = np::FloatArray2(width, height);
	phase = np::FloatArray2(width, height);

	// 2D Fourier transform objects
	fft2r_pd.initialize(2 * width, 2 * height);
	fft2c_pd.initialize(2 * width, 2 * height);
	fft2r.initialize(width, height);
	fft2c.initialize(width, height);

	// Initialize QPI reconstruction buffers
	init_qpi_buffs(pupil_radius, reg_l2_a, reg_l2_ph, reg_tv);
}

QpiProcess::~QpiProcess()
{
}


void QpiProcess::init_qpi_buffs(float pupil_radius, float _reg_l2_a, float _reg_l2_ph, float _reg_tv)
{
	// Generate x, y mesh grid
	int diameter = width;
	int diameter2 = 2 * diameter;
	int radius = diameter / 2;

	np::FloatArray2 x_map(diameter2, diameter2);
	np::FloatArray2 y_map(diameter2, diameter2);

	Ipp32f* horizontal_line = ippsMalloc_32f(diameter2);
	Ipp32f* vertical_line = ippsMalloc_32f(diameter2);
	ippsVectorSlope_32f(horizontal_line, diameter2, (Ipp32f)+2*radius, -1.0f);

	for (int i = 0; i < diameter2; i++)
	{
		ippsSet_32f((float)(i - 2*radius), vertical_line, diameter2);
		memcpy(x_map.raw_ptr() + i * diameter2, horizontal_line, sizeof(float) * diameter2);
		memcpy(y_map.raw_ptr() + i * diameter2, vertical_line, sizeof(float) * diameter2);
	}
	ippsFree(horizontal_line);
	ippsFree(vertical_line);

	// Rho map
	np::FloatArray2 rho(diameter2, diameter2);
	ippsMagnitude_32f(x_map, y_map, rho, diameter2 * diameter2);
	ippsMulC_32f_I(((Ipp32f)radius*2.0f - 1.0f) / (2*radius), rho, diameter2 * diameter2);

	// Pupil image
	np::Uint8Array2 pupil(diameter2, diameter2);
	ippiCompareC_32f_C1R(rho, sizeof(float) * diameter2, pupil_radius, pupil, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippCmpLessEq);
	ippsDivC_8u_ISfs(255, pupil, pupil.length(), 0);

	// FFT shift
	np::Uint8Array2 pupil_shift(diameter2, diameter2);
	ippiCopy_8u_C1R(&pupil(0, 0), sizeof(uint8_t) * diameter2, &pupil_shift(diameter, diameter), sizeof(uint8_t) * diameter2, { diameter, diameter });
	ippiCopy_8u_C1R(&pupil(diameter, diameter), sizeof(uint8_t) * diameter2, &pupil_shift(0, 0), sizeof(uint8_t) * diameter2, { diameter, diameter });
	ippiCopy_8u_C1R(&pupil(0, diameter), sizeof(uint8_t) * diameter2, &pupil_shift(diameter, 0), sizeof(uint8_t) * diameter2, { diameter, diameter });
	ippiCopy_8u_C1R(&pupil(diameter, 0), sizeof(uint8_t) * diameter2, &pupil_shift(0, diameter), sizeof(uint8_t) * diameter2, { diameter, diameter });

	// Fourier transform of pupil
	np::FloatArray2 pupil_shift_32f(diameter2, diameter2);
	np::ComplexFloatArray2 pupil_ft(diameter2, diameter2);
	ippsConvert_8u32f(pupil_shift, pupil_shift_32f, pupil_shift_32f.length());
	fft2r_pd((Ipp32fc*)pupil_ft.raw_ptr(), pupil_shift_32f.raw_ptr());

	// Generate source image for calculation of phase transfer function
	tbb::parallel_for(tbb::blocked_range<size_t>(0, 4),
		[&, x_map, y_map, pupil_shift, pupil_ft](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			np::Uint8Array2 source(diameter2, diameter2);
			switch (i)
			{
			case top:
				//ippiCompareC_32f_C1R(y_map, sizeof(float) * diameter2, 0, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippCmpGreaterEq);
				ippiScale_32f8u_C1R(y_map, sizeof(float) * diameter2, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, -pupil_radius, +pupil_radius);
				ippiMirror_8u_C1IR(source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippAxsHorizontal);
				break;
			case left:
				//ippiCompareC_32f_C1R(x_map, sizeof(float) * diameter2, 0, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippCmpLess);
				ippiScale_32f8u_C1R(x_map, sizeof(float) * diameter2, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, -pupil_radius, +pupil_radius);
				break;
			case bottom:
				//	ippiCompareC_32f_C1R(y_map, sizeof(float) * diameter2, 0, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippCmpLess);
				ippiScale_32f8u_C1R(y_map, sizeof(float) * diameter2, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, -pupil_radius, +pupil_radius);
				break;
			case right:
				//ippiCompareC_32f_C1R(x_map, sizeof(float) * diameter2, 0, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippCmpGreaterEq);
				ippiScale_32f8u_C1R(x_map, sizeof(float) * diameter2, source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, -pupil_radius, +pupil_radius);
				ippiMirror_8u_C1IR(source, sizeof(uint8_t) * diameter2, { diameter2, diameter2 }, ippAxsVertical);
				break;
			}
					
			np::Uint8Array2 source_shift(diameter2, diameter2);
			ippiCopy_8u_C1R(&source(0, 0), sizeof(uint8_t) * diameter2, &source_shift(diameter, diameter), sizeof(uint8_t) * diameter2, { diameter, diameter });
			ippiCopy_8u_C1R(&source(diameter, diameter), sizeof(uint8_t) * diameter2, &source_shift(0, 0), sizeof(uint8_t) * diameter2, { diameter, diameter });
			ippiCopy_8u_C1R(&source(0, diameter), sizeof(uint8_t) * diameter2, &source_shift(diameter, 0), sizeof(uint8_t) * diameter2, { diameter, diameter });
			ippiCopy_8u_C1R(&source(diameter, 0), sizeof(uint8_t) * diameter2, &source_shift(0, diameter), sizeof(uint8_t) * diameter2, { diameter, diameter });

			///ippsDivC_8u_ISfs(255, source, source.length(), 0);
			///ippsMul_8u_ISfs(pupil_shift, source, source.length(), 0);
			ippsMul_8u_ISfs(pupil_shift, source_shift, source_shift.length(), 0);

			// Fourier transform of each source image
			np::FloatArray2 source_32f(diameter2, diameter2);
			np::ComplexFloatArray2 source_ft(diameter2, diameter2);
			ippsConvert_8u32f(source_shift, source_32f, source_32f.length());
			fft2r_pd((Ipp32fc*)source_ft.raw_ptr(), source_32f.raw_ptr(), i);
			ippsConj_32fc_I((Ipp32fc*)source_ft.raw_ptr(), source_ft.length());
			
			// Calculate phase transfer function of each illumination pattern
			np::ComplexFloatArray2 fsp(diameter2, diameter2);
			np::FloatArray2 fsp_real(diameter2, diameter2), fsp_imag(diameter2, diameter2);
			ippsMul_32fc((const Ipp32fc*)pupil_ft.raw_ptr(), (const Ipp32fc*)source_ft.raw_ptr(), (Ipp32fc*)fsp.raw_ptr(), fsp.length());
			
			ippsCplxToReal_32fc((const Ipp32fc*)fsp.raw_ptr(), fsp_real, fsp_imag, fsp.length());
			memset(fsp_real, 0, sizeof(float) * fsp_real.length());
			ippsRealToCplx_32f(fsp_real, fsp_imag, (Ipp32fc*)fsp.raw_ptr(), fsp.length());

			np::ComplexFloatArray2 _ph_H(diameter2, diameter2);
			fft2c_pd.inverse((Ipp32fc*)_ph_H.raw_ptr(), (const Ipp32fc*)fsp.raw_ptr(), i);

			float dc;			
			ippsSum_32f(source_32f.raw_ptr(), source_32f.length(), &dc, ippAlgHintNone);			
			
			np::FloatArray2 _ph_H_real(diameter2, diameter2), _ph_H_imag(diameter2, diameter2);
			np::FloatArray2 _ph_H_real0(diameter, diameter), _ph_H_imag0(diameter, diameter);
			ippsCplxToReal_32fc((const Ipp32fc*)_ph_H.raw_ptr(), _ph_H_real, _ph_H_imag, _ph_H.length());

			ippsMulC_32f_I(+2.0f / dc, _ph_H_real, _ph_H_real.length());
			ippsMulC_32f_I(-2.0f / dc, _ph_H_imag, _ph_H_imag.length());

			ippiCopy_32f_C1R(&_ph_H_real(0, 0), sizeof(float) * diameter2, &_ph_H_real0(0, 0), sizeof(float) * diameter, { radius, radius });
			ippiCopy_32f_C1R(&_ph_H_real(3 * radius, 0), sizeof(float) * diameter2, &_ph_H_real0(radius, 0), sizeof(float) * diameter, { radius, radius });
			ippiCopy_32f_C1R(&_ph_H_real(0, 3 * radius), sizeof(float) * diameter2, &_ph_H_real0(0, radius), sizeof(float) * diameter, { radius, radius });
			ippiCopy_32f_C1R(&_ph_H_real(3 * radius, 3 * radius), sizeof(float) * diameter2, &_ph_H_real0(radius, radius), sizeof(float) * diameter, { radius, radius });

			ippiCopy_32f_C1R(&_ph_H_imag(0, 0), sizeof(float) * diameter2, &_ph_H_imag0(0, 0), sizeof(float) * diameter, { radius, radius });
			ippiCopy_32f_C1R(&_ph_H_imag(3 * radius, 0), sizeof(float) * diameter2, &_ph_H_imag0(radius, 0), sizeof(float) * diameter, { radius, radius });
			ippiCopy_32f_C1R(&_ph_H_imag(0, 3 * radius), sizeof(float) * diameter2, &_ph_H_imag0(0, radius), sizeof(float) * diameter, { radius, radius });
			ippiCopy_32f_C1R(&_ph_H_imag(3 * radius, 3 * radius), sizeof(float) * diameter2, &_ph_H_imag0(radius, radius), sizeof(float) * diameter, { radius, radius });
						
			ippsRealToCplx_32f(_ph_H_imag0, _ph_H_real0, (Ipp32fc*)ph_H.at(i).raw_ptr(), ph_H.at(i).length());
			ippsConj_32fc((const Ipp32fc*)ph_H.at(i).raw_ptr(), (Ipp32fc*)ph_Hc.at(i).raw_ptr(), ph_Hc.at(i).length());
		}
	});

	QFile file("phH.data");
	if (file.open(QIODevice::WriteOnly))
	{
		for (int i = 0; i < 4; i++)
			file.write(reinterpret_cast<char*>(ph_H.at(i).raw_ptr()), sizeof(float) * 2 * ph_H.at(i).length());

		file.close();
	}

	// Generate regularization terms
	np::FloatArray2 temp(diameter, diameter), Dx_pow(diameter, diameter), Dy_pow(diameter, diameter); 
	np::ComplexFloatArray2 Dx(diameter, diameter), Dy(diameter, diameter);
	memset(temp, 0, sizeof(float) * temp.length());
	temp(0, 0) = reg_tv; temp(0, diameter - 1) = -reg_tv;
	fft2r((Ipp32fc*)Dx.raw_ptr(), temp);
	memset(temp, 0, sizeof(float) * temp.length());
	temp(0, 0) = reg_tv; temp(diameter - 1, 0) = -reg_tv;
	fft2r((Ipp32fc*)Dy.raw_ptr(), temp);

	ippsPowerSpectr_32fc((const Ipp32fc*)Dx.raw_ptr(), Dx_pow, Dx_pow.length());
	ippsPowerSpectr_32fc((const Ipp32fc*)Dy.raw_ptr(), Dy_pow, Dy_pow.length());
	ippsAdd_32f(Dx_pow, Dy_pow, tv_term, tv_term.length());
	
	// Generate reconstruction buffers
	memcpy(M11, tv_term, sizeof(float) * tv_term.length());
	ippsAddC_32f_I(reg_l2_a, M11, M11.length());

	memcpy(M22, tv_term, sizeof(float) * tv_term.length());
	ippsAddC_32f_I(reg_l2_ph, M22, M22.length());
	for (int i = 0; i < 4; i++)
	{
		ippsPowerSpectr_32fc((const Ipp32fc*)ph_H.at(i).raw_ptr(), temp, temp.length());
		ippsAdd_32f_I(temp, M22, M22.length());
	}

	ippsMul_32f(M11, M22, denom, denom.length());
	ippsDivCRev_32f_I(1.0f, denom, denom.length());

	//QFile file("phH.data");
	//if (file.open(QIODevice::WriteOnly))
	//{
	//	for (int i = 0; i < 4; i++)
	//		file.write(reinterpret_cast<char*>(ph_H.at(i).raw_ptr()), sizeof(float) * 2 * ph_H.at(i).length());

	//	file.close();
	//}
}


void QpiProcess::getBrightfield(std::vector<np::FloatArray2>& _compo)
{	
	memset(brightfield, 0, sizeof(float) * brightfield.length());
	for (int i = 0; i < 4; i++)
		ippsAdd_32f_I(_compo.at(i), brightfield, brightfield.length());
	ippsDivC_32f_I(4.0f, brightfield, brightfield.length());
}

void QpiProcess::getDpc(std::vector<np::FloatArray2>& _compo, dpc_orientation orientation)
{	
	ippsSub_32f(_compo.at(0 + orientation), _compo.at(2 + orientation), _sub, _sub.length());
	ippsAdd_32f(_compo.at(0 + orientation), _compo.at(2 + orientation), _add, _add.length());

	switch (orientation)
	{
	case top_bottom:
		ippsDiv_32f(_add, _sub, dpc_tb, dpc_tb.length());
		break;
	case left_right:
		ippsDiv_32f(_add, _sub, dpc_lr, dpc_lr.length());
		break;
		///case fusion:
		///np::FloatArray2 dpc_tb2(CMOS_WIDTH, CMOS_HEIGHT), dpc_lr2(CMOS_WIDTH, CMOS_HEIGHT), dpc_fusion(CMOS_WIDTH, CMOS_HEIGHT);
		///ippsMul_32f(dpc_tb, dpc_tb, dpc_tb2, dpc_tb2.length());
		///ippsMul_32f(dpc_lr, dpc_lr, dpc_lr2, dpc_lr2.length());
		///ippsAdd_32f(dpc_tb2, dpc_lr2, dpc_fusion, dpc_fusion.length());
		///ippsSqrt_32f_I(dpc_fusion, dpc_fusion.length());
		///break;
	}
}

void QpiProcess::getQpi(std::vector<np::FloatArray2>& _compo)
{
	//QFile file("compo.data");
	//if (file.open(QIODevice::WriteOnly))
	//{
	//	for (int i = 0; i < 4; i++)
	//		file.write(reinterpret_cast<char*>(_compo.at(i).raw_ptr()), sizeof(float) * _compo.at(i).length());

	//	file.close();
	//}

	// Allocate buffers for Fourier transform of component images
	std::vector<np::ComplexFloatArray2> _compo_ft;
	for (int i = 0; i < 4; i++)
	{
		np::ComplexFloatArray2 _ft(width, height);
		_compo_ft.push_back(_ft);
	}

	// Fourier transform of each component image
	tbb::parallel_for(tbb::blocked_range<size_t>(0, 4),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{			
			// Normalize component image
			float mean;
			np::FloatArray2 norm_compo(width, height);
			ippsMean_32f(_compo.at(i), _compo.at(i).length(), &mean, ippAlgHintFast);
			ippsDivC_32f_I(mean, _compo.at(i), _compo.at(i).length());
			ippsSubC_32f_I(-1.0f, _compo.at(i), _compo.at(i).length());

			// Fourier transform
			fft2r((Ipp32fc*)_compo_ft.at(i).raw_ptr(), _compo.at(i).raw_ptr(), i);
		}
	});

	// Reconstruction
	np::ComplexFloatArray2 temp(width, height), I2(width, height), cplx_ph(width, height);
	memset(I2, 0, sizeof(float) * 2 * I2.length());
	for (int i = 0; i < 4; i++)
	{
		ippsMul_32fc((const Ipp32fc*)_compo_ft.at(i).raw_ptr(), (const Ipp32fc*)ph_Hc.at(i).raw_ptr(), (Ipp32fc*)temp.raw_ptr(), temp.length());
		ippsAdd_32fc_I((const Ipp32fc*)temp.raw_ptr(), (Ipp32fc*)I2.raw_ptr(), I2.length());
	}
		
	ippsMul_32f32fc_I(M11, (Ipp32fc*)I2.raw_ptr(), I2.length());
	ippsMul_32f32fc_I(denom, (Ipp32fc*)I2.raw_ptr(), I2.length());
	
	fft2c.inverse((Ipp32fc*)cplx_ph.raw_ptr(), (const Ipp32fc*)I2.raw_ptr());
	ippsReal_32fc((const Ipp32fc*)cplx_ph.raw_ptr(), phase, phase.length());
}