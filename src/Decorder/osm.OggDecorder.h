
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <math.h>

#include <vorbis/vorbisfile.h>

#include "../OpenSoundMixer.h"
#include "../osm.Decorder.h"

namespace osm
{
	class OggBuffer
	{
	private:
		uint8_t*	m_data = nullptr;
		int32_t		m_size = 0;
		int32_t		m_current = 0;

	public:
		OggBuffer(uint8_t* data, int32_t size);
		static size_t read(void* buffer, size_t size, size_t maxCount, void* stream);
		static int seek(void* buffer, ogg_int64_t offset, int flag);
		static long tell(void* buffer);
		static int close(void* buffer);
	};

	class OggDecorder
		: public Decorder
	{
	private:
		std::shared_ptr<OggBuffer>	m_oggBuffer;
		OggVorbis_File				m_ovf;
		bool						m_loaded = false;

		struct
		{
			std::vector<uint8_t>		Samples;
			int32_t						CurrentSample;
			int32_t						TotalSample;
			int32_t						SamplePerSec;
			int32_t						ChannelCount;
		} m_original;

	public:
		OggDecorder();
		virtual ~OggDecorder();
		bool Load(uint8_t* data, int32_t size) override;
		int32_t GetSamples(Sample* samples, int32_t offset, int32_t count) override;
		int32_t GetSampleCount() override;
	};
}
