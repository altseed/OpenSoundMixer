
#include "OpenSoundMixer.h"

#if defined(_OTHER)
#include "Manager/osm.Manager_Impl_Other.h"
#elif _WIN32
#if defined(OSM_OUTPUT_XAUDIO2)
#include "Manager/osm.Manager_Impl_XAudio2.h"
#elif defined(OSM_OUTPUT_WASAPI)
#include "Manager/osm.Manager_Impl_WasApi.h"
#endif
#elif defined(__APPLE__)
#include "Manager/osm.Manager_Impl_OpenAL.h"
#else
#include "Manager/osm.Manager_Impl_PulseAudio.h"
#endif

namespace osm
{
	Manager* Manager::Create()
	{
#if defined(_OTHER)
		auto manager = new osm::Manager_Impl_Other();
#elif _WIN32
#if defined(OSM_OUTPUT_XAUDIO2)
		auto manager = new osm::Manager_Impl_XAudio2();
#elif defined(OSM_OUTPUT_WASAPI)
		auto manager = new osm::Manager_Impl_WasApi();
#endif
#elif defined(__APPLE__)
		auto manager = new osm::Manager_Impl_OpenAL();
#else
		auto manager = new osm::Manager_Impl_PulseAudio();
#endif
		return manager;
	}
}