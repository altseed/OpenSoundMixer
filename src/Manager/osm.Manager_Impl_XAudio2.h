
#pragma once

#include "../osm.Manager_Impl.h"

#include <thread>
#include <xaudio2.h>

namespace osm
{
	class Manager_Impl_XAudio2
		: public Manager_Impl
	{
	private:
		IXAudio2*				m_xaudio;
		IXAudio2MasteringVoice*	m_masteringVoice;
		IXAudio2SourceVoice*	m_sourceVoice;
		volatile bool			m_threading;
		std::thread				m_thread;

		void Reset();

		static void ThreadFunc(void* p);

	public:
		Manager_Impl_XAudio2();
		virtual ~Manager_Impl_XAudio2();

		bool InitializeInternal() override;
		void FinalizeInternal() override;
	};
}