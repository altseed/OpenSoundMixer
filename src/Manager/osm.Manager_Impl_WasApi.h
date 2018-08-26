
#pragma once

#include "../osm.Manager_Impl.h"
#include "../Filter/osm.Resampler.h"

#include <thread>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <functiondiscoverykeys.h>

namespace osm
{
	class Manager_Impl_WasApi
		: public Manager_Impl
	{
	private:
		IMMDeviceEnumerator*	m_context = nullptr;
		IMMDevice*				m_device = nullptr;
		IAudioClient*			m_audioClient = nullptr;
		IAudioRenderClient*		m_audioRender = nullptr;
		WAVEFORMATEXTENSIBLE	m_format;
		Resampler				m_resampler;

		volatile bool			m_threading;
		std::thread				m_thread;

		HANDLE                  m_event = NULL;
		HANDLE					m_audioProcessingDoneEvent = NULL;
		bool					initializingCo = false;

		void Reset();

		static void ThreadFunc(void* p);

	public:
		Manager_Impl_WasApi();
		virtual ~Manager_Impl_WasApi();

		bool InitializeInternal() override;
		void FinalizeInternal() override;
	};
}