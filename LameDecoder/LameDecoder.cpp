// LameDecoder.cpp
#include "pch.h"
#include "LameDecoder.h"
#include "SpectrumAnalyzerXAPO.h"

using namespace LameDecoder;
using namespace Platform;
using namespace concurrency;


namespace LameDecoder
{
	struct StreamingVoiceCallback : public IXAudio2VoiceCallback
	{
	private:
		DecoderComponent ^m_decoder;

	public:
		StreamingVoiceCallback(DecoderComponent ^player){m_decoder=player;}

		STDMETHOD_( void, OnVoiceProcessingPassStart )( UINT32 bytesRequired )
		{
			//unblock render thread
			//SetEvent(m_decoder->m_musicRenderThreadHandle);
		}

		STDMETHOD_( void, OnVoiceProcessingPassEnd )(){}
		STDMETHOD_( void, OnStreamEnd )(){}
		STDMETHOD_( void, OnBufferStart )( void* pContext ){}
		STDMETHOD_( void, OnBufferEnd )( void* pContext ){}
		STDMETHOD_( void, OnLoopEnd )( void* pContext ){}
		STDMETHOD_( void, OnVoiceError )( void* pContext, HRESULT error ){}
	private:
	};

}


/* copy stereo samples */
#define COPY_STEREO(DST_TYPE, SRC_TYPE)                                                         \
	DST_TYPE *pcm_l = (DST_TYPE *)pcm_l_raw, *pcm_r = (DST_TYPE *)pcm_r_raw;                    \
	SRC_TYPE const *p_samples = (SRC_TYPE const *)p;                                            \
	for (i = 0; i < processed_samples; i++) {                                                   \
	*pcm_l++ = (DST_TYPE)(*p_samples++);                                                      \
	*pcm_r++ = (DST_TYPE)(*p_samples++);                                                      \
	}

DecoderComponent::DecoderComponent()
{


}
void DecoderComponent::Initialize()
{
	gfp=lame_init();
	int const v_main = 2 - lame_get_version(gfp);
	int ret = lame_init_params(gfp);
	int decodeOnly=lame_get_decode_only(gfp);

	//XAudio2 Init
	m_musicMasteringVoice=nullptr;
	m_musicSourceVoice=nullptr;
	callback=nullptr;
	pSpectrumAnalyzerXAPO=nullptr;

	ZeroMemory(&Wav,sizeof(Wav));

	ThrowIfFailed(
		XAudio2Create(&m_musicEngine, 0)
		);
#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfiguration = { 0 };
	debugConfiguration.TraceMask = XAUDIO2_LOG_WARNINGS;
	debugConfiguration.BreakMask = XAUDIO2_LOG_ERRORS;
	m_musicEngine->SetDebugConfiguration( &debugConfiguration );
#endif

	ThrowIfFailed(m_musicEngine->CreateMasteringVoice(
		&m_musicMasteringVoice,
		XAUDIO2_DEFAULT_CHANNELS,
		44100, //usually
		0,
		nullptr,  
		nullptr,  
		AudioCategory_BackgroundCapableMedia
		));

	m_RenderThreadExitHandle=CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, WRITE_OWNER | EVENT_ALL_ACCESS);
	ResetEvent(m_RenderThreadExitHandle);

}

void DecoderComponent::Cleanup()
{
	if (m_musicSourceVoice!=nullptr)
	{
		m_musicSourceVoice->DestroyVoice();
		//set exit flag from render thread
		m_bExitFromRenderThreadFlag=TRUE;
		SetEvent(m_musicRenderThreadHandle);
		//wait for render thread exit
		WaitForSingleObjectEx(m_RenderThreadExitHandle,INFINITE,TRUE);
		ResetEvent(m_RenderThreadExitHandle);
		m_musicSourceVoice=nullptr;
		CloseHandle(m_musicRenderThreadHandle);
	}
	if (callback!=nullptr)
	{
		delete callback;
		callback=nullptr;
	}

	if (pSpectrumAnalyzerXAPO!=nullptr)
	{
		assert(pSpectrumAnalyzerXAPO->Release()==0);
		pSpectrumAnalyzerXAPO=nullptr;
	}

	if (m_musicMasteringVoice!=nullptr)
	{
		m_musicMasteringVoice->DestroyVoice();
		m_musicMasteringVoice=nullptr;
	}

	if (m_musicEngine!=nullptr)
	{
		assert(m_musicEngine->Release()==0);
		m_musicEngine=nullptr;
	};
}

DecoderComponent::~DecoderComponent(void)
{



}

#define OUTSIZE_UNCLIPPED (1152*2*sizeof(FLOAT))

void DecoderComponent::SetBytestream(IRandomAccessStream^ streamHandle)
{
	/*concurrency::create_async([this,streamHandle]()
	{*/
	Streams::Buffer^ bb = ref new Streams::Buffer(streamHandle->Size);
	create_task(streamHandle->ReadAsync(bb,streamHandle->Size,InputStreamOptions::None)).wait();
	auto reader = ::Windows::Storage::Streams::DataReader::FromBuffer(bb);

	Microsoft::WRL::ComPtr< Windows::Storage::Streams::IBufferByteAccess > buffer_byte_access;
	reinterpret_cast< IUnknown* >( bb )->QueryInterface( IID_PPV_ARGS( &buffer_byte_access ) ) ;
	byte* mp3 = nullptr;
	buffer_byte_access->Buffer(&mp3);

	hip_t hip = hip_decode_init();

	mp3data_struct mp3data;
	memset(&mp3data, 0, sizeof(mp3data));

	int nChannels = -1;
	int nSampleRate = -1;
	int read, i, samples;
	int mp3_len, debug_count, read_count = 0, total_frames = 0;
	const int PCM_SIZE = 1152;
	short int pcm_l[PCM_SIZE], pcm_r[PCM_SIZE];
	static const int BUFFER_COUNT=3;


	//int numdec=hip_decode1_headers(decoder,mp3,streamHandle->Size,pcm_l_buf,pcm_r_buf,&mp3data);
	mp3_len = streamHandle->Size;
	debug_count = 0;
	bool parsed_headers=false;
	do 
	{
		samples = hip_decode1_headers(hip, mp3, mp3_len, pcm_l, pcm_r, &mp3data);
		if(mp3data.header_parsed == 1)
		{
			if(nChannels < 0)
				printf("header parsed. channels=%d, samplerate=%d\n", mp3data.stereo, mp3data.samplerate);
			else
			{
				if(nChannels != mp3data.stereo || nSampleRate != mp3data.samplerate)
					printf("channels changed. channels=%d->%d, samplerate=%d->%d\n",nChannels, mp3data.stereo, nSampleRate, mp3data.samplerate);
			}
			nChannels = mp3data.stereo;
			nSampleRate = mp3data.samplerate;		
			if (!parsed_headers)
			{
				parsed_headers=true;
				InitializeXAudio2(mp3data.stereo, mp3data.samplerate,mp3data.bitrate);
			}

		}
		if(samples > 0 && mp3data.header_parsed != 1)
		{
			fprintf(stderr, "lame decode error, samples=%d, but header not parsed yet\n", samples);
			break;
		}
		if(samples > 0)
		{
			for(i = 0; i < samples; i++)
			{
				/*		fwrite((char*)&pcm_l[i], 1, sizeof(pcm_l[i]), pcm);
				if(nChannels == 2)
				fwrite((char*)&pcm_r[i], 1, sizeof(pcm_r[i]), pcm);*/
			}

			XAUDIO2_VOICE_STATE state;
			//queue is full, skip and wait again
			do 
			{
				WaitForSingleObjectEx(m_musicRenderThreadHandle,INFINITE,TRUE);
				m_musicSourceVoice->GetState(&state);
				if (state.BuffersQueued>=BUFFER_COUNT)
					ResetEvent(m_musicRenderThreadHandle);
			}while(state.BuffersQueued>=BUFFER_COUNT);

			XAUDIO2_BUFFER abuf = {0};
			abuf.AudioBytes = samples;
			abuf.pAudioData =(byte*) &pcm_l;

			ThrowIfFailed(m_musicSourceVoice->SubmitSourceBuffer(&abuf));

			debug_count++;
			if(samples % mp3data.framesize != 0)
				printf("error: not mod samples. samples=%d, framesize=%d\n", samples, mp3data.framesize);
			total_frames += (samples / mp3data.framesize);
		}
		/* future calls to decodeMP3 are just to flush buffers */
		mp3_len = 0;
	} while (samples > 0);
	hip_decode_exit(hip);
	//});

}

void DecoderComponent::InitializeXAudio2(WORD Channels,DWORD SamplesPerSec,WORD BitsPerSample)
{
	WAVEFORMATEX  waveFormat; 
	waveFormat.wFormatTag = WAVE_FORMAT_PCM; 
	waveFormat.nChannels = 1; 
	waveFormat.nSamplesPerSec = SamplesPerSec; 
	waveFormat.wBitsPerSample = 16; 
	waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample/8); 
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign; 
	waveFormat.cbSize = 0; 

	//initialize pWAV

	callback=new StreamingVoiceCallback (this);

	ThrowIfFailed(
		m_musicEngine->CreateSourceVoice(
		&m_musicSourceVoice, &waveFormat, 0, 1.0f, callback, nullptr)
		);

	pSpectrumAnalyzerXAPO = new SpectrumAnalyzerXAPO();

	XAUDIO2_EFFECT_DESCRIPTOR descriptor;
	descriptor.InitialState = true;
	descriptor.OutputChannels = waveFormat.nChannels;
	descriptor.pEffect = pSpectrumAnalyzerXAPO;


	XAUDIO2_EFFECT_CHAIN chain;
	chain.EffectCount = 1;
	chain.pEffectDescriptors = &descriptor;


	ThrowIfFailed(
		m_musicSourceVoice->SetEffectChain(&chain)
		);

	ThrowIfFailed(
		m_musicSourceVoice->DisableEffect(0, 0)
		);



	if (m_musicSourceVoice!=nullptr)
		ThrowIfFailed(m_musicSourceVoice->Start());
}



//std::vector<unsigned char> DecoderComponent::getData( ::Windows::Storage::Streams::IBuffer^ buf )
//{
//    auto reader = ::Windows::Storage::Streams::DataReader::FromBuffer(buf);
//
//    std::vector<unsigned char> data(reader->UnconsumedBufferLength);
//
//    if ( !data.empty() )
//        reader->ReadBytes(
//            ::Platform::ArrayReference<unsigned char>(
//                &data[0], data.size()));
//
//    return data;
//}