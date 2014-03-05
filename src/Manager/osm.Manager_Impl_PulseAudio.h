
#pragma once

#include "../osm.Manager_Impl.h"

#include <thread>

namespace osm
{
	class Manager_Impl_PulseAudio
		: public Manager_Impl
	{
	private:

		volatile bool			m_threading;
		std::thread				m_thread;

		void Reset();

		static void ThreadFunc(void* p);

	public:
		Manager_Impl_PulseAudio();
		virtual ~Manager_Impl_PulseAudio();

		bool InitializeInternal() override;
		void FinalizeInternal() override;
	};
}