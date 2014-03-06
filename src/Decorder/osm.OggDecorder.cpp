
#include "osm.OggDecorder.h"

#include <vorbis/vorbisfile.h>

namespace osm
{
	OggBuffer::OggBuffer(uint8_t* data, int32_t size)
		: m_data(data)
		, m_size(size)
	{
	}

	size_t OggBuffer::read(void* buffer, size_t size, size_t maxCount, void* stream)
	{
		if (buffer == nullptr)
		{
			return 0;
		}

		auto p = static_cast<OggBuffer*>(stream);

		int32_t rsize = p->m_size - p->m_current;
		size_t count = rsize / size;
		if (count > maxCount)
		{
			count = maxCount;
		}

		memcpy(buffer, p->m_data + p->m_current, size * count);
		p->m_current += size * count;

		return count;
	}

	int OggBuffer::seek(void* buffer, ogg_int64_t offset, int flag)
	{
		auto p = static_cast<OggBuffer*>(buffer);

		switch (flag)
		{
		case SEEK_CUR:
			p->m_current += offset;
			break;

		case SEEK_END:
			p->m_current = p->m_size + offset;
			break;

		case SEEK_SET:
			p->m_current = offset;
			break;

		default:
			return -1;
		}

		if (p->m_current > p->m_size)
		{
			p->m_current = p->m_size;
			return -1;
		}
		else if (p->m_current < 0)
		{
			p->m_current = 0;
			return -1;
		}

		return 0;
	}

	long OggBuffer::tell(void* buffer)
	{
		auto p = static_cast<OggBuffer*>(buffer);
		return p->m_current;
	}

	int OggBuffer::close(void* buffer)
	{
		return 0;
	}

	OggDecorder::OggDecorder()
	{
		m_original.CurrentSample = 0;
		m_original.TotalSample = 0;
		m_original.SamplePerSec = 0;
		m_original.ChannelCount = 0;
	}

	OggDecorder::~OggDecorder()
	{
		if (m_loaded)
		{
			ov_clear(&m_ovf);
		}
	}

	bool OggDecorder::Load(uint8_t* data, int32_t size)
	{
		m_oggBuffer = std::make_shared<OggBuffer>(data, size);

		ov_callbacks callbacks =
		{
			&OggBuffer::read,
			&OggBuffer::seek,
			&OggBuffer::close,
			&OggBuffer::tell
		};

		if (ov_open_callbacks(m_oggBuffer.get(), &m_ovf, 0, 0, callbacks) != 0)
		{
			return false;
		}

		if (ov_seekable(&m_ovf) == 0)
		{
			ov_clear(&m_ovf);
			return false;
		}

		auto info = ov_info(&m_ovf, -1);

		m_original.SamplePerSec = info->rate;
		m_original.ChannelCount = info->channels;

		m_original.TotalSample = ov_pcm_total(&m_ovf, -1);
		m_loaded = true;

		return true;
	}

	int32_t OggDecorder::GetSamples(Sample* samples, int32_t offset, int32_t count)
	{
		auto e2iSample = [this](int32_t e) -> double
		{
			return ((double) e / 44100.0) * (double) m_original.SamplePerSec;
		};

		auto sampleStart = e2iSample(offset);
		auto sampleEnd = e2iSample(offset + count);
		
		auto sampleStart_i = (int32_t) sampleStart;
		auto sampleEnd_i = ((int32_t) sampleEnd) == sampleEnd ? (int32_t) sampleEnd : (int32_t) (sampleEnd + 1);

		sampleStart_i = sampleStart_i >= m_original.TotalSample ? m_original.TotalSample : sampleStart_i;
		sampleEnd_i = sampleEnd_i >= m_original.TotalSample ? m_original.TotalSample : sampleEnd_i;

		int32_t sampleSize = 2 * m_original.ChannelCount;

		if (m_original.CurrentSample > sampleStart_i)
		{
			m_original.Samples.clear();
			ov_pcm_seek(&m_ovf, sampleStart_i);
			m_original.CurrentSample = sampleStart_i;
		}

		while (m_original.CurrentSample + m_original.Samples.size() / sampleSize <= sampleEnd_i)
		{
			uint8_t buffer[4096];
			int bitstream = 0;
			int readSize = 0;

			readSize = ov_read(
				&m_ovf,
				(char*)buffer,
				4096,
				0,
				2,
				1,
				&bitstream
				);

			for (auto i = 0; i < readSize; i++)
			{
				m_original.Samples.push_back(buffer[i]);
			}

			// 末尾？
			if (m_original.CurrentSample + m_original.Samples.size() / sampleSize == m_original.TotalSample) break;
		}

		auto data = (int16_t*)m_original.Samples.data();
		auto sampleCount = m_original.Samples.size() / sampleSize;
		
		for (int32_t i = 0; i < count; i++)
		{
			auto sample = e2iSample(offset + i) - m_original.CurrentSample;
			auto sampleR = (int32_t) sample;
			auto sampleD = sample - sampleR;

			int16_t left1, left2, right1, right2;

			if (sampleR >= sampleCount)
			{
				m_original.CurrentSample += i;

				// 使用済み削除
				m_original.CurrentSample = sampleEnd_i;
				m_original.Samples.clear();

				return i;
			}
			else if (sampleR >= sampleCount - 1)
			{
				left1 = data[sampleR * m_original.ChannelCount + 0];
				if (m_original.ChannelCount == 2)
				{
					right1 = data[sampleR * m_original.ChannelCount + 1];
				}
				else
				{
					right1 = left1;
				}

				samples[i].Left = left1 * (1.0 - sampleD);
				samples[i].Right = right1 * (1.0 - sampleD);

				// 使用済み削除
				m_original.CurrentSample = sampleEnd_i;
				m_original.Samples.clear();

				return i + 1;
			}
			else
			{
				left1 = data[sampleR * m_original.ChannelCount + 0];
				left2 = data[(sampleR + 1) * m_original.ChannelCount + 0];

				if (m_original.ChannelCount == 2)
				{
					right1 = data[sampleR * m_original.ChannelCount + 1];
					right2 = data[(sampleR + 1) * m_original.ChannelCount + 1];
				}
				else
				{
					right1 = left1;
					right2 = left2;
				}

				samples[i].Left = left1 * (1.0 - sampleD) + left2 * (sampleD);
				samples[i].Right = right1 * (1.0 - sampleD) + right2 * (sampleD);
			}
		}

		// 使用済み削除
		auto removingSize = (sampleEnd_i - 1 - m_original.CurrentSample) * sampleSize;
		m_original.Samples.erase(m_original.Samples.begin(), m_original.Samples.begin() + removingSize);
		m_original.CurrentSample = sampleEnd_i - 1;

		return count;
	}

	int32_t OggDecorder::GetSampleCount()
	{
		auto frameCount = (double) m_original.TotalSample / (double) m_original.SamplePerSec * 44100.0;

		return (int32_t) frameCount;
	}
}