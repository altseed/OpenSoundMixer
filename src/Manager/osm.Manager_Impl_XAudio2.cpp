
#include "osm.Manager_Impl_XAudio2.h"

namespace osm
{
	void Manager_Impl_XAudio2::Reset()
	{
		if (m_threading)
		{
			m_threading = false;
			m_thread.join();
		}

		if (m_sourceVoice != nullptr)
		{
			m_sourceVoice->Stop();
			m_sourceVoice->DestroyVoice();
			m_sourceVoice = nullptr;
		}

		if (m_masteringVoice != nullptr)
		{
			m_masteringVoice->DestroyVoice();
			m_masteringVoice = nullptr;
		}

		if (m_xaudio != nullptr)
		{
			m_xaudio->Release();
			m_xaudio = nullptr;
		}
	}

	void Manager_Impl_XAudio2::ThreadFunc(void* p)
	{
		Manager_Impl_XAudio2* this_ = (Manager_Impl_XAudio2*) p;

		const int32_t bufferDivision = 100;

		Sample bufs[4][44100 / bufferDivision];
		int32_t targetBuf = 0;

		int32_t current = 0;

		while (this_->m_threading)
		{
			XAUDIO2_VOICE_STATE state;
			this_->m_sourceVoice->GetState(&state);
			if (state.BuffersQueued < 2)
			{
				this_->ReadSamples(bufs[targetBuf], 44100 / bufferDivision);

				XAUDIO2_BUFFER xbuf = { 0 };
				xbuf.AudioBytes = 44100 / bufferDivision * sizeof(Sample);
				xbuf.pAudioData = (uint8_t*) bufs[targetBuf];
				xbuf.Flags = XAUDIO2_END_OF_STREAM;
				targetBuf = (targetBuf + 1) % 4;

				this_->m_sourceVoice->SubmitSourceBuffer(&xbuf);
			}
			else
			{
				Sleep(1);
			}
		}

		// I‚í‚è‚Ü‚Å‘Ò‚Â
		while (true)
		{
			XAUDIO2_VOICE_STATE state;
			this_->m_sourceVoice->GetState(&state);
			if (state.BuffersQueued != 0)
			{
				Sleep(1);
			}
			else
			{
				break;
			}
		}
	}

	Manager_Impl_XAudio2::Manager_Impl_XAudio2()
		: m_xaudio(nullptr)
		, m_masteringVoice(nullptr)
		, m_sourceVoice(nullptr)
		, m_threading(false)
	{

	}

	Manager_Impl_XAudio2::~Manager_Impl_XAudio2()
	{
		Reset();
	}

	bool Manager_Impl_XAudio2::InitializeInternal()
	{
		::CoInitialize(0);

		HRESULT hr;
		WAVEFORMATEX format = { 0 };

		uint32_t flags = 0;
#ifdef _DEBUG
		flags |= XAUDIO2_DEBUG_ENGINE;
#endif
		hr = XAudio2Create(&m_xaudio, flags);
		if (FAILED(hr))
		{
			printf("Failed XAudio2Create\n");
			goto End;
		}

		hr = m_xaudio->CreateMasteringVoice(&m_masteringVoice);
		if (FAILED(hr))
		{
			printf("Failed CreateMasteringVoice\n");
			goto End;
		}

		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2;
		format.wBitsPerSample = 16;
		format.nSamplesPerSec = 44100;
		format.nBlockAlign = format.wBitsPerSample / 8 * format.nChannels;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

		hr = m_xaudio->CreateSourceVoice(&m_sourceVoice, &format);
		if (FAILED(hr))
		{
			printf("Failed CreateSourceVoice\n");
			goto End;
		}

		m_sourceVoice->Start();

		m_threading = true;
		m_thread = std::thread(ThreadFunc, this);
		return true;

	End:;
		Reset();
		::CoUninitialize();
		return false;
	}

	void Manager_Impl_XAudio2::FinalizeInternal()
	{
		Reset();
		::CoUninitialize();
	}
}