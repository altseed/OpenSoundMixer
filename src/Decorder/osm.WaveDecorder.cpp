
#include "osm.WaveDecorder.h"

namespace osm
{
	PCM::PCM(void* data, int32_t size, int32_t channelCount, int32_t samplesPerSec, int32_t bitsPerSample)
		: m_data((uint8_t*) data)
		, m_size(size)
		, m_channelCount(channelCount)
		, m_samplesPerSec(samplesPerSec)
		, m_bitsPerSample(bitsPerSample)
	{
	}

	double PCM::GetLength() const
	{
		int32_t frameByte = m_channelCount * (m_bitsPerSample / 8);

		return (double) m_size / (double) frameByte / (double) m_samplesPerSec;
	}

	int32_t PCM::GetSampleCountAs44100Stereo16bit() const
	{
		auto length = GetLength();
		return length * 44100;
	}

	void PCM::GetSampleAs44100Stereo16bit(int32_t frame, Sample& sample) const
	{
		auto u8tos16 = [](uint8_t u) -> int16_t
		{
			auto u_ = (double) u;
			u_ = (u_ - 128.0) / 127.0;
			auto s_ = u_ * 32767.0;
			if (s_ > 32767) s_ = 32767;
			if (s_ < -32768) s_ = -32768;
			return (int16_t) s_;
		};

		double time = (double) frame / 44100.0;
		double originalFrame = time * m_samplesPerSec;

		int32_t originalFrameR = (int32_t) originalFrame;
		double originalFrameD = originalFrame - originalFrameR;

		int32_t originalFrameByte = m_channelCount * (m_bitsPerSample / 8);

		if (originalFrameR >= m_size / originalFrameByte)
		{
			sample.Left = 0;
			sample.Right = 0;
			return;
		}
		else if (originalFrameR >= m_size / originalFrameByte - 1)
		{
			auto data = &(m_data[originalFrameR * originalFrameByte]);
			if (m_channelCount == 2)
			{
				if (m_bitsPerSample == 16)
				{
					auto d = (int16_t*) data;
					sample.Left = d[0];
					sample.Right = d[1];
				}
				else
				{
					sample.Left = u8tos16(data[0]);
					sample.Right = u8tos16(data[1]);
				}
			}
			else
			{
				if (m_bitsPerSample == 16)
				{
					auto d = (int16_t*) data;
					sample.Left = d[0];
					sample.Right = sample.Left;
				}
				else
				{
					sample.Left = u8tos16(data[0]);
					sample.Right = sample.Left;
				}
			}

			sample.Left = (1.0 - originalFrameD) * sample.Left;
			sample.Right = (1.0 - originalFrameD) * sample.Right;
		}
		else
		{
			auto data = &(m_data[originalFrameR* originalFrameByte]);
			short left1, left2, right1, right2;
			if (m_channelCount == 2)
			{
				if (m_bitsPerSample == 16)
				{
					auto d = (int16_t*) data;
					left1 = d[0];
					right1 = d[1];
					left2 = d[2];
					right2 = d[3];
				}
				else
				{
					left1 = u8tos16(data[0]);
					right1 = u8tos16(data[1]);
					left2 = u8tos16(data[2]);
					right2 = u8tos16(data[3]);
				}
			}
			else
			{
				if (m_bitsPerSample == 16)
				{
					auto d = (int16_t*) data;
					left1 = d[0];
					left2 = d[1];
					right1 = left1;
					right2 = left2;
				}
				else
				{
					left1 = u8tos16(data[0]);
					left2 = u8tos16(data[1]);
					right1 = left1;
					right2 = left2;
				}
			}

			sample.Left = (1.0 - originalFrameD) * left1 + originalFrameD * left2;
			sample.Right = (1.0 - originalFrameD) * right1 + originalFrameD * right2;
		}
	}

	bool WaveDecorder::Read(void* dst, void* src, int copySize, int& offset, int dataSize)
	{
		if (offset + copySize > dataSize) return false;
		uint8_t* s = (uint8_t*) src + offset;

		memcpy(dst, s, copySize);

		offset += copySize;

		return true;
	}

	WaveDecorder::WaveDecorder()
	{
	
	}

	WaveDecorder::~WaveDecorder()
	{

	}

	bool WaveDecorder::Load(uint8_t* data, int32_t size)
	{
		int32_t offset = 0;
		WAVEFormat fmt;

		char riff[4];
		if (!Read(riff, data, 4, offset, size)) return false;

		int32_t riffSize = 0;
		if (!Read(&riffSize, data, sizeof(int32_t), offset, size)) return false;

		char wave[4];
		if (!Read(wave, data, 4, offset, size)) return false;

		while (true)
		{
			char chunk[5];
			chunk[4] = 0;
			if (!Read(chunk, data, 4, offset, size)) break;

			int32_t chunkSize = 0;

			if (STRICMP("fmt ", chunk) == 0)
			{
				// チャンクサイズ
				if (!Read(&chunkSize, data, sizeof(int32_t), offset, size)) return false;

				// フォーマット
				if (!Read(&fmt, data, 16, offset, size)) return false;
				offset += (chunkSize - 16);

				if (fmt.FormatID == 1)
				{
					// PCMの場合
				}
				else
				{
					// それ以外
					return false;
				}
			}
			else if (STRICMP("data", chunk) == 0)
			{
				// チャンクサイズ
				if (!Read(&chunkSize, data, sizeof(int32_t), offset, size)) return false;

				// データ
				m_data.resize(chunkSize);
				if (!Read(m_data.data(), data, chunkSize, offset, size)) return false;

				if (fmt.FormatID == 1)
				{
					// PCMの場合
					auto pcm = new PCM(m_data.data(), m_data.size(), fmt.ChannelCount, fmt.SamplesPerSec, fmt.BitsPerSample);
					m_pcm = std::shared_ptr<PCM>(pcm);
				}
			}
			else
			{
				// チャンクサイズ
				if (!Read(&chunkSize, data, sizeof(int32_t), offset, size)) return false;

				// 不明
				offset += chunkSize;
			}
		}

		return true;
	}

	int32_t WaveDecorder::GetSamples(Sample* samples, int32_t offset, int32_t count)
	{
		if (offset + count >= GetSampleCount())
		{
			count = GetSampleCount() - offset;
		}

		for (int32_t i = 0; i < count; i++)
		{
			m_pcm->GetSampleAs44100Stereo16bit(i + offset, samples[i]);
		}

		return count;
	}

	int32_t WaveDecorder::GetSampleCount()
	{
		return m_pcm->GetSampleCountAs44100Stereo16bit();
	}
}
