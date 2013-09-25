//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#include "pch.h"
#include "SpectrumAnalyzerXAPO.h"
#include "xdsp.h"
#include "FFTSampleAggregator.h"

XAPO_REGISTRATION_PROPERTIES reg=
{
	__uuidof(SpectrumAnalyzerXAPO),
	L"SpectrumAnalyzerXAPO",
	L"Copyright (C)2013 Microsoft Corporation",
	1,
	0,
	XAPO_FLAG_INPLACE_REQUIRED
	| XAPO_FLAG_CHANNELS_MUST_MATCH
	| XAPO_FLAG_FRAMERATE_MUST_MATCH
	| XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH
	| XAPO_FLAG_BUFFERCOUNT_MUST_MATCH
	| XAPO_FLAG_INPLACE_SUPPORTED,
	1, 1, 1, 1
};
SpectrumAnalyzerXAPO::SpectrumAnalyzerXAPO():CXAPOBase(&reg)
{
	sampleAggregator=nullptr;
	uChannels=0;
	uBytesPerSample=0;
	nSamplesPerSec=0;

	cnt=0;
}



SpectrumAnalyzerXAPO::~SpectrumAnalyzerXAPO(void)
{
	if (sampleAggregator!=nullptr)
		delete sampleAggregator;
}


HRESULT SpectrumAnalyzerXAPO::LockForProcess (
	UINT32 InputLockedParameterCount,
	const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS  *pInputLockedParameters,
	UINT32 OutputLockedParameterCount,
	const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS  *pOutputLockedParameters
	)
{
	assert(!IsLocked());
	assert(InputLockedParameterCount == 1);
	assert(OutputLockedParameterCount == 1);
	assert(pInputLockedParameters != NULL);
	assert(pOutputLockedParameters != NULL);
	assert(pInputLockedParameters[0].pFormat != NULL);
	assert(pOutputLockedParameters[0].pFormat != NULL);


	uChannels = pInputLockedParameters[0].pFormat->nChannels;
	nSamplesPerSec=pInputLockedParameters[0].pFormat->nSamplesPerSec;
	uBytesPerSample = 
		(pInputLockedParameters[0].pFormat->wBitsPerSample >> 3);
	
	if (sampleAggregator!=nullptr)
	{
		delete sampleAggregator;
	}
	sampleAggregator=new FFTSampleAggregator(uChannels);


	return CXAPOBase::LockForProcess(
		InputLockedParameterCount,
		pInputLockedParameters,
		OutputLockedParameterCount,
		pOutputLockedParameters);
}

void SpectrumAnalyzerXAPO::Process(
	UINT32 InputProcessParameterCount,
	const XAPO_PROCESS_BUFFER_PARAMETERS *pInputProcessParameters,
	UINT32 OutputProcessParameterCount,
	XAPO_PROCESS_BUFFER_PARAMETERS *pOutputProcessParameters,
	BOOL IsEnabled
	)
{
		assert(IsLocked());
		assert(InputProcessParameterCount == 1);
		assert(OutputProcessParameterCount == 1);
		assert(NULL != pInputProcessParameters);
		assert(NULL != pOutputProcessParameters);


		XAPO_BUFFER_FLAGS inFlags = pInputProcessParameters[0].BufferFlags;
		XAPO_BUFFER_FLAGS outFlags = pOutputProcessParameters[0].BufferFlags;

		assert(inFlags == XAPO_BUFFER_VALID || 
			inFlags == XAPO_BUFFER_SILENT);
		assert(outFlags == XAPO_BUFFER_VALID || 
			outFlags == XAPO_BUFFER_SILENT);


		void* pvSrc = pInputProcessParameters[0].pBuffer;
		assert(pvSrc != NULL);

		void* pvDst = pOutputProcessParameters[0].pBuffer;
		assert(pvDst != NULL);


		switch (inFlags)
		{
		case XAPO_BUFFER_VALID:
			{
				//this XAPO doest really modify input signal
				// so either copy of source to dest doesn't require
				//memcpy(pvDst,pvSrc,pInputProcessParameters[0].ValidFrameCount * 
				// uChannels * uBytesPerSample);
				//
				// fun synth, example above
				// WARNING: VERY SLOW CODE
				/*
				for (int m=0;m<pInputProcessParameters[0].ValidFrameCount;m++)
				{
					for (int y=0;y<uChannels;y++)
					{
						float freq=440;
						double m_sampleTick=(DirectX::XM_PI*2.0f)/nSamplesPerSec;

						float *sample_ptr=(float *)pvDst+(m*uChannels);
						sample_ptr[y]=sin( (freq*m_sampleTick)*cnt);
					}
					cnt++;
				}*/
		
				// filtro iir: http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
				/*
				
				You specified the following parameters:
				filtertype  =  Chebyshev
				passtype  =  Bandstop
				ripple  =  -3
				order  =  2
				samplerate  =  44100
				corner1  =  120
				corner2  =  11050
				adzero  =
				logmin  =  -110

				Results
				Command line: /www/usr/fisher/helpers/mkfilter -Ch -3.0000000000e+00 -Bs -o 2 -a 2.7210884354e-03 2.5056689342e-01
				raw alpha1    =   0.0027210884
				raw alpha2    =   0.2505668934
				warped alpha1 =   0.0027211547
				warped alpha2 =   0.3194456970
				gain at dc    :   mag = 3.273502783e+00   phase =   0.0000000000 pi
				gain at centre:   mag = 4.055683743e-01   phase =   0.9093864459 pi
				gain at hf    :   mag = 3.273502783e+00   phase =   0.0000000000 pi

				S-plane zeros:
				0.0000000000 + j   0.1852486311	2 times
				0.0000000000 + j  -0.1852486311	2 times

				S-plane poles:
				-0.9009262845 + j  -2.1979548006
				-0.9009262845 + j   2.1979548006
				-0.0054791647 + j   0.0133673049
				-0.0054791647 + j  -0.0133673049

				Z-plane zeros:
				0.9829874277 + j   0.1836728533	2 times
				0.9829874277 + j  -0.1836728533	2 times

				Z-plane poles:
				-0.1240092499 + j  -0.6637149261
				-0.1240092499 + j   0.6637149261
				0.9944471967 + j   0.0132937725
				0.9944471967 + j  -0.0132937725

				Recurrence relation:
				y[n] = (  1 * x[n- 4])
				+ ( -3.9319497107 * x[n- 3])
				+ (  5.8650571319 * x[n- 2])
				+ ( -3.9319497107 * x[n- 1])
				+ (  1 * x[n- 0])

				+ ( -0.4509274227 * y[n- 4])
				+ (  0.6614130129 * y[n- 3])
				+ ( -0.9517151450 * y[n- 2])
				+ (  1.7408758937 * y[n- 1])
				*/

				for (unsigned int i = 0; i < pInputProcessParameters[0].ValidFrameCount; i++)
				{
					// voice cancellation
					static float y_minus_4 = 0;
					static float y_minus_3 = 0;
					static float y_minus_2 = 0;
					static float y_minus_1 = 0;

					static float x_minus_4 = 0;
					static float x_minus_3 = 0;
					static float x_minus_2 = 0;
					static float x_minus_1 = 0;
					
					if (IsEnabled) {
						float ra = ((float*) pvSrc)[i*uChannels];
						float la = ((float*) pvSrc)[i*uChannels + 1];
						float nv = ra - la;

						float x = ra ; 
						float y;

						y = (1 * x_minus_4)
							+ (-3.9319497107  * x_minus_3)
							+ (5.8650571319  * x_minus_2)
							+ (-3.9319497107 * x_minus_1)
							+ (1 * x )

							+ (-0.4509274227  * y_minus_4)
							+ (0.6614130129  * y_minus_3)
							+ (-0.9517151450 * y_minus_2)
							+ (1.7408758937 * y_minus_1);

						// updates previous samples pointers
						y_minus_4 = y_minus_3;
						y_minus_3 = y_minus_2;
						y_minus_2 = y_minus_1;
						y_minus_1 = y;

						x_minus_4 = x_minus_3;
						x_minus_3 = x_minus_2;
						x_minus_2 = x_minus_1;
						x_minus_1 = x;

						// updates in/out
						((float*) pvSrc)[i*uChannels] = ((float*) pvDst)[i*uChannels] = nv + y / 3.27;
						((float*) pvSrc)[i*uChannels] = ((float*) pvDst)[i*uChannels + 1] = nv + y / 3.27;
					}
					sampleAggregator->Add((float *) pvSrc + i*uChannels);
				}
			}

		case XAPO_BUFFER_SILENT:
			{
				// All that needs to be done for this case is setting the
				// output buffer flag to XAPO_BUFFER_SILENT which is done below.
				break;
			}
		}

		pOutputProcessParameters[0].ValidFrameCount = 
			pInputProcessParameters[0].ValidFrameCount; 
		pOutputProcessParameters[0].BufferFlags     = 
			pInputProcessParameters[0].BufferFlags;
}


Platform::Array<float> ^SpectrumAnalyzerXAPO::GetFFT()
{
	int spectrumsamples=FFTSampleAggregator::fft_samples_count/2;
	float result[4096];
	assert(sizeof(result)>spectrumsamples);

	this->sampleAggregator->FFT(result);
	Platform::Array<float> ^arr=ref new Platform::Array<float>
		(spectrumsamples);
	CopyMemory(arr->Data,result,spectrumsamples*sizeof(float));
	
	return arr;
}