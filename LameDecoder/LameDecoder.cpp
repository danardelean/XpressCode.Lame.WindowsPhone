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
	:m_isPlaying(false),novoice(false)
{


}


void DecoderComponent::Initialize(int Channels,int SamplesPerSec,int BitRate)
{

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


	WAVEFORMATEX  waveFormat; 
	waveFormat.wFormatTag = WAVE_FORMAT_PCM; 
	waveFormat.nChannels = Channels; 
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
		novoice?m_musicSourceVoice->EnableEffect(0, 0):m_musicSourceVoice->DisableEffect(0, 0)
		);


	if (m_musicSourceVoice!=nullptr)
		ThrowIfFailed(m_musicSourceVoice->Start());


	gfp=lame_init();
	//int const v_main = 2 - lame_get_version(gfp);
	int ret = lame_init_params(gfp);
	int decodeOnly=lame_get_decode_only(gfp);

	hip = hip_decode_init();

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
		//WaitForSingleObjectEx(m_RenderThreadExitHandle,INFINITE,TRUE);
		//ResetEvent(m_RenderThreadExitHandle);
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

	hip_decode_exit(hip);
}

DecoderComponent::~DecoderComponent(void)
{



}

static const int BUFFER_COUNT=5;

bool DecoderComponent::Play(const Platform::Array<byte>^ bytes,int Channels,int SamplesPerSec,int BitRate)
{
	if (m_isPlaying)
		return false;

	Initialize(Channels,SamplesPerSec,BitRate);

	m_isPlaying=true;

	int mp3_len;
	mp3_len=bytes->Length;
	byte* mp3=(byte*)malloc(sizeof(byte)*mp3_len);
	memcpy(mp3,bytes->Data,sizeof(byte)*mp3_len);	


	concurrency::create_async([this,mp3,mp3_len]()
	{
		int nChannels = -1;
		int nSampleRate = -1;
		int read, i, samples;
		int debug_count, read_count = 0, total_frames = 0;
		const int PCM_SIZE = 1152;
		short int pcm_l[PCM_SIZE], pcm_r[PCM_SIZE], pcm_mixed[10][PCM_SIZE*2];

		debug_count = 0;
		samples = 0;
		int position = 0;
		int current_buffer = 0;

		mp3data_struct mp3data;
		memset(&mp3data, 0, sizeof(mp3data));

		do 
		{
			samples = hip_decode1_headers(hip, mp3 + position, 0, pcm_l, pcm_r, &mp3data);
			if (samples == 0) {
				samples = hip_decode1_headers(hip, mp3 + position, position < mp3_len ? 1024 : mp3_len - position -1, pcm_l, pcm_r, &mp3data);
				position +=1024;
			}
			//	samples = hip_decode1_headers(hip, mp3 , position < mp3_len ? 1024 : mp3_len - position -1, pcm_l, pcm_r, &mp3data);

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
			}
			if(samples > 0 && mp3data.header_parsed != 1)
			{
				fprintf(stderr, "lame decode error, samples=%d, but header not parsed yet\n", samples);
				break;
			}
			if((samples > 0)&&(m_isPlaying))
			{
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
				//if(nChannels == 2)
				abuf.AudioBytes = samples * sizeof(pcm_l[0]) *2;
				for(int j=0; j< samples;j++) {
					pcm_mixed[current_buffer][j*2]=pcm_l[j];
					pcm_mixed[current_buffer][j*2+1]=pcm_r[j];
				}
				abuf.pAudioData =(byte*) pcm_mixed[current_buffer];

				current_buffer++;
				current_buffer%=10;

				ThrowIfFailed(m_musicSourceVoice->SubmitSourceBuffer(&abuf));

				debug_count++;
				if(samples % mp3data.framesize != 0)
					printf("error: not mod samples. samples=%d, framesize=%d\n", samples, mp3data.framesize);
				total_frames += (samples / mp3data.framesize);
			}
		} while ((samples > 0 || position < mp3_len)&&m_isPlaying);
		Cleanup();

	});
	return true;
}


void DecoderComponent::Stop()
{
	if (!m_isPlaying)
		return;
	
	m_isPlaying=false;
}




//void DecoderComponent::SetBytestream(IRandomAccessStream^ streamHandle)
//{
//	concurrency::create_async([this,streamHandle]()
//	{
//		Streams::Buffer^ bb = ref new Streams::Buffer(streamHandle->Size);
//		create_task(streamHandle->ReadAsync(bb,streamHandle->Size,InputStreamOptions::None)).wait();
//		auto reader = ::Windows::Storage::Streams::DataReader::FromBuffer(bb);
//
//		Microsoft::WRL::ComPtr< Windows::Storage::Streams::IBufferByteAccess > buffer_byte_access;
//		reinterpret_cast< IUnknown* >( bb )->QueryInterface( IID_PPV_ARGS( &buffer_byte_access ) ) ;
//		byte* mp3 = nullptr;
//		buffer_byte_access->Buffer(&mp3);
//
//
//
//
//
//
//
//		//int numdec=hip_decode1_headers(decoder,mp3,streamHandle->Size,pcm_l_buf,pcm_r_buf,&mp3data);
//
//
//
//
//	});
//
//}



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