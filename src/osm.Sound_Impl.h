
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#include "OpenSoundMixer.h"
#include "osm.Decorder.h"
#include "osm.ReferenceObject.h"

namespace osm
{
	class Sound_Impl
		: public Sound
		, public ReferenceObject
	{
	private:
		bool						m_isDecompressed;
		std::vector<Sample>			m_samples;

		std::vector<uint8_t>		m_data;
		std::shared_ptr<Decorder>	m_decorder;
	
	public:
		Sound_Impl();
		~Sound_Impl();

		bool Load(void* data, int32_t size, bool isDecompressed);
		int32_t GetSamples(Sample* samples, int32_t offset, int32_t count);
		int32_t GetSampleCount();

		// IReferenceを継承したデバイスオブジェクト向け定義
#if !SWIG
	public:
		virtual int GetRef() { return ReferenceObject::GetRef(); }
		virtual int AddRef() { return ReferenceObject::AddRef(); }
		virtual int Release() { return ReferenceObject::Release(); }
#endif
	};
}