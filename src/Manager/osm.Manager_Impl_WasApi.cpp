
#include "osm.Manager_Impl_WasApi.h"

namespace osm
{
	void Manager_Impl_WasApi::Reset()
	{
		if (m_threading)
		{
			m_threading = false;
			m_thread.join();
		}

		if (m_audioRender)
		{
			m_audioRender->Release();
			m_audioRender = nullptr;
		}

		if (m_audioClient)
		{
			m_audioClient->Stop();
			m_audioClient->Release();
			m_audioClient = nullptr;
		}
	}

	void Manager_Impl_WasApi::ThreadFunc(void* p)
	{
		Manager_Impl_WasApi* this_ = (Manager_Impl_WasApi*) p;

		const int32_t bufferDivision = 100;

		bool requiredResampling = (this_->m_resampler.GetResampleRatio() != 1.0f);

		while (this_->m_threading)
		{
			uint32_t bufferFrames = 0;
			this_->m_audioClient->GetBufferSize(&bufferFrames);
			
			Sample* outputBuffer = nullptr;
			this_->m_audioRender->GetBuffer(bufferFrames, (BYTE**)&outputBuffer);
			if (outputBuffer == NULL) {
				Sleep(1);
				continue;
			}

			const uint32_t unitSize = 44100 / bufferDivision;
			int32_t outputCount = 0;
			Sample buffer[unitSize];

			while (bufferFrames - outputCount >= unitSize * 2)
			{
				if (requiredResampling)
				{
					int32_t sampleCount = this_->ReadSamples(buffer, unitSize);
					auto result = this_->m_resampler.ProcessSamples(buffer, sampleCount, 
						outputBuffer + outputCount, bufferFrames - outputCount);
					outputCount += result.second;
				}
				else
				{
					int32_t sampleCount = this_->ReadSamples(outputBuffer + outputCount, unitSize);
					outputCount += sampleCount;
				}
			}

			this_->m_audioRender->ReleaseBuffer((uint32_t)outputCount, 0);
		}
	}

	Manager_Impl_WasApi::Manager_Impl_WasApi()
		: m_threading(false)
	{

	}

	Manager_Impl_WasApi::~Manager_Impl_WasApi()
	{
		Reset();
	}

	bool Manager_Impl_WasApi::InitializeInternal()
	{
		HRESULT hr;

		hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

		initializingCo = SUCCEEDED(hr);

		// Get the audio endpoint device enumerator.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			 CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_context);
		if (FAILED(hr)) {
			return false;
		}

		hr = m_context->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_device);
		if (FAILED(hr)) {
			return false;
		}
		
		hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_audioClient);
		if (FAILED(hr)) {
			return false;
		}

		// Get the device latency
		REFERENCE_TIME defaultDevicePeriod = 0;
		REFERENCE_TIME minimumDevicePeriod = 0;
		hr = m_audioClient->GetDevicePeriod(&defaultDevicePeriod, &minimumDevicePeriod);
		if (FAILED(hr)) {
			return false;
		}

		// Get the device format
		WAVEFORMATEX* deviceFormat = nullptr;
		hr = m_audioClient->GetMixFormat(&deviceFormat);
		if (FAILED(hr)) {
			return false;
		}

		// Set the output format
		m_format.Format = *deviceFormat;
		m_format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		m_format.Format.nChannels = 2;
		m_format.Format.wBitsPerSample = 16;
		//m_format.Format.nSamplesPerSec = 48000;	// Need to use device sample rate
		m_format.Format.nBlockAlign = m_format.Format.nChannels * m_format.Format.wBitsPerSample / 8;
		m_format.Format.nAvgBytesPerSec = m_format.Format.nBlockAlign * m_format.Format.nSamplesPerSec;
		m_format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
		m_format.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		m_format.Samples.wValidBitsPerSample = m_format.Format.wBitsPerSample;
		m_format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

		hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 
			defaultDevicePeriod, defaultDevicePeriod, &m_format.Format, NULL);
		if (FAILED(hr)) {
			return false;
		}

		hr = m_audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_audioRender);
		if (FAILED(hr)) {
			return false;
		}

		// Set sample rate converter
		m_resampler.SetResampleRatio(m_format.Format.nSamplesPerSec / 44100.0);
		
		// Start device
		m_audioClient->Start();
		
		m_threading = true;
		m_thread = std::thread(ThreadFunc, this);
		SetThreadPriority((HANDLE)m_thread.native_handle(), THREAD_PRIORITY_HIGHEST);
		
		return true;
	}

	void Manager_Impl_WasApi::FinalizeInternal()
	{
		Reset();

		SafeRelease(m_context);

		if (initializingCo)
		{
			::CoUninitialize();
		}
	}
}