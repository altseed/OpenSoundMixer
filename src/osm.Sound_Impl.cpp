
#include "osm.Sound_Impl.h"

#include "Decorder/osm.OggDecorder.h"
#include "Decorder/osm.WaveDecorder.h"

namespace osm
{
	Sound_Impl::Sound_Impl()
		: m_isDecompressed(false)
	{
		m_loopStart = 0.0f;
		m_loopEnd = -1.0;
	}

	Sound_Impl::~Sound_Impl()
	{
	}

	bool Sound_Impl::Load(void* data, int32_t size, bool isDecompressed)
	{
		if (data == nullptr) return false;
		if (size <= 0) return false;

		m_data.resize(size);
		memcpy(m_data.data(), data, size);

		auto type = Decorder::GetFileType(data, size);

		if (type == eFileType::OGG)
		{
			m_decorder = std::make_shared<OggDecorder>();
		}
		else if (type == eFileType::WAVE)
		{
			m_decorder = std::make_shared<WaveDecorder>();
		}

		auto loaded = m_decorder->Load((uint8_t*) m_data.data(), size);
		if (!loaded) return false;

		if (isDecompressed)
		{
			m_samples.resize(m_decorder->GetSampleCount());
			auto count = m_decorder->GetSamples(m_samples.data(), 0, m_samples.size());
			assert(m_samples.size() == count);

			m_decorder.reset();
			m_data.resize(0);
			m_data.shrink_to_fit();

			m_isDecompressed = true;
		}
		else
		{
			m_isDecompressed = false;
		}

		if (m_loopEnd == -1.0f)
		{
			m_loopEnd = GetLength();
		}

		return true;
	}

	int32_t Sound_Impl::GetSamples(Sample* samples, int32_t offset, int32_t count)
	{
		if (m_isDecompressed)
		{
			count = Min(m_samples.size() - offset, count);
			memcpy(samples, &(m_samples[offset]), count * sizeof(Sample));
			return count;
		}

		return m_decorder->GetSamples(samples, offset, count);
	}

	int32_t Sound_Impl::GetSampleCount() const
	{
		if (m_isDecompressed) return m_samples.size();

		return m_decorder->GetSampleCount();
	}

	float Sound_Impl::GetLength() const
	{
		return GetSampleCount() / 44100.0f;
	}
}