
#include "OpenSoundMixer.h"

#if _WIN32
#include "Manager/osm.Manager_Impl_XAudio2.h"
#else
#include "Manager/osm.Manager_Impl_PulseAudio.h"
#endif

namespace osm
{
	Manager* Manager::Create()
	{
#if _WIN32
		auto manager = new osm::Manager_Impl_XAudio2();
#else
		auto manager = new osm::Manager_Impl_PulseAudio();
#endif
		return manager;
	}
}