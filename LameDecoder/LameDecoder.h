#pragma once

using namespace std;

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

class SpectrumAnalyzerXAPO;
namespace LameDecoder
{
	struct StreamingVoiceCallback;
	public ref class DecoderComponent sealed
	{
	public:
		DecoderComponent();
		virtual ~DecoderComponent(void);
		bool Play(const Platform::Array<byte>^ bytes,int Channels,int SamplesPerSec,int BitRate);
		void Stop();
		property Platform::String^ Version;
		property bool NoVoice 
		{
			bool get(){ 
				return novoice;
			}
			void set(bool data) 
			{
				novoice=data;
				if (m_musicSourceVoice!=nullptr)
					ThrowIfFailed(novoice?m_musicSourceVoice->EnableEffect(0, 0):m_musicSourceVoice->DisableEffect(0, 0));
			}
		}
	private:
		void Initialize(int Channels,int SamplesPerSec,int BitRate);
		void Cleanup();
		bool novoice;
		lame_global_flags* gfp;
		IXAudio2 *m_musicEngine;
		IXAudio2MasteringVoice *m_musicMasteringVoice;
		IXAudio2SourceVoice * m_musicSourceVoice;
		WAVEFORMATEX Wav;
		SpectrumAnalyzerXAPO *pSpectrumAnalyzerXAPO;
		StreamingVoiceCallback *callback;
		HANDLE m_musicRenderThreadHandle;
		HANDLE m_RenderThreadExitHandle;
		BOOL m_bExitFromRenderThreadFlag;
		size_t BufferLength;
		hip_t hip;
		int m_BufferCount;
		bool m_isPlaying;
	
	};
	//read header
	/*MP3Hdr hdr = { 0 };
	fread(reinterpret_cast<char *>(&hdr), 1,sizeof(hdr),mp3);
	if (0 != ::memcmp(hdr.tag, "ID3", 3))
        throw std::invalid_argument("Not an MP3 File");
	if (0 != (hdr.flags&0x40))
    {
        fin.seekg(sizeof(MP3ExtHdr), std::ifstream::cur);
        if (!fin.good())
            throw std::invalid_argument("Error reading file");
    }*/

	struct MP3Hdr {
		char tag[3];
		unsigned char maj_ver;
		unsigned char min_ver;
		unsigned char flags;
		unsigned int  size;
	};
	struct MP3ExtHdr {
		unsigned int  size;
		unsigned char num_flag_bytes;
		unsigned char extended_flags;
	};
	struct MP3FrameHdr {
		char frame_id[4];
		unsigned size;
		unsigned char flags[2];
	};
}