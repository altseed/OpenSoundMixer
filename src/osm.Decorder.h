
#pragma once

#include "OpenSoundMixer.h"

namespace osm
{
	enum class eFileType
	{
		Unknown,
		OGG,
		WAVE,
	};

	class Decorder
	{
	public:
		Decorder() = default;
		virtual ~Decorder() = default;
		virtual bool Load(uint8_t* data, int32_t size) = 0;
		virtual int32_t GetSamples(Sample* samples, int32_t offset, int32_t count) = 0;
		virtual int32_t GetSampleCount() = 0;

		static eFileType GetFileType(void* data, int32_t size);
	};
}