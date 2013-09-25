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
		void SetBytestream(IRandomAccessStream^ streamHandle);
		void Initialize();
		void Cleanup();
		property Platform::String^ Version;
	private:
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
		void InitializeXAudio2(WORD Channels,DWORD SamplesPerSec,WORD BitRate);
    };
}