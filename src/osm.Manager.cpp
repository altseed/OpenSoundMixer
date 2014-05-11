
#include "OpenSoundMixer.h"

#if _WIN32
#include "Manager/osm.Manager_Impl_XAudio2.h"
#elif defined(__APPLE__)

#else
#include "Manager/osm.Manager_Impl_PulseAudio.h"
#endif

namespace osm
{
	Manager* Manager::Create()
	{
#if _WIN32
		auto manager = new osm::Manager_Impl_XAudio2();
#elif defined(__APPLE__)
		return nullptr;
#else
		auto manager = new osm::Manager_Impl_PulseAudio();
#endif
		return manager;
	}
}