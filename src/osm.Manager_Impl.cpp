
#include "osm.Manager_Impl.h"
#include "osm.Sound_Impl.h"
#include "Filter/osm.Panner.h"

namespace osm
{
	int32_t Manager_Impl::ReadSamples(Sample* samples, int32_t sampleCount)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		// 合成処理
		int32_t tempBufferLen = sampleCount * 5; // 比率は再生速度の上限倍数+α
		m_tempSamples.resize(tempBufferLen);

		memset(samples, 0, sizeof(Sample) * sampleCount);

		// TODO: 複数音源で再生速度を変えた場合に出力サンプルが欠損する可能性あり
		int32_t minActualOut = sampleCount;

		for (auto& s : m_soundStates)
		{
			if (s.second.IsPaused) continue;

			int32_t inCount = sampleCount;

			Resampler *resampler = nullptr;
			if (s.second.SoundPtr->GetIsPlaybackSpeedEnabled())
			{
				if (!s.second.ResamplerPtr) {
					s.second.ResamplerPtr = std::make_shared<Resampler>();
				}
				resampler = &*s.second.ResamplerPtr;
			}

			if (resampler)
			{
				double playbackSpeed = s.second.SoundPtr->GetPlaybackSpeed();
				double ratio = 1.0 / playbackSpeed;
				resampler->SetResampleRatio(ratio);
				int32_t exceedance = resampler->GetInputExceedance();
				inCount = std::min(tempBufferLen, (int32_t)ceil(sampleCount / ratio) + 1) - (exceedance / 16);
			}

			int32_t rest = inCount;
			int32_t writingPos = 0;

			auto loopStart = (int32_t)(s.second.SoundPtr->GetLoopStartingPoint() * 44100);
			auto loopEnd = (int32_t) (s.second.SoundPtr->GetLoopEndPoint() * 44100);
			auto enabledLoop = s.second.SoundPtr->GetIsLoopingMode();

			if (loopEnd > s.second.SoundPtr->GetSampleCount()) loopEnd = s.second.SoundPtr->GetSampleCount();
			if (loopStart == loopEnd) enabledLoop = false;
			if (loopStart > loopEnd) enabledLoop = false;
			if (loopEnd < 0) enabledLoop = false;

			while (rest > 0)
			{
				int32_t readSize = rest;

				// loop
				if (enabledLoop)
				{
					if (s.second.SamplePos + rest >= loopEnd)
					{
						readSize = loopEnd - s.second.SamplePos;
					}
				}

				auto size = s.second.SoundPtr->GetSamples(&m_tempSamples[writingPos], s.second.SamplePos, readSize);
				if (size < 0) { // エラー発生したのでとりあえず停止
					s.second.SamplePos = s.second.SoundPtr->GetSampleCount() + 1;
					break;
				}

				s.second.SamplePos += size;
				writingPos += size;

				rest -= size;

				// loop
				if (enabledLoop)
				{
					if (s.second.SamplePos >= loopEnd)
					{
						s.second.SamplePos = loopStart;
					}
				}
				else
				{
					if (s.second.SamplePos >= s.second.SoundPtr->GetSampleCount())
					{
						break;
					}
				}
			}

			if (writingPos < inCount) {
				for (int32_t i = writingPos; i < inCount; ++i) {
					m_tempSamples[i].Left = 0;
					m_tempSamples[i].Right = 0;
				}
			}

			int32_t actualOut = inCount;
			if (resampler)
			{
				double ratio = resampler->GetResampleRatio();
				m_resampleBuf.resize(sampleCount);
				auto resampledCount = resampler->ProcessSamples(m_tempSamples.data(), inCount, m_resampleBuf.data(), sampleCount);
				actualOut = resampledCount.second;
				memcpy(m_tempSamples.data(), m_resampleBuf.data(), actualOut * sizeof(Sample));
			}

			float panningPosition = s.second.SoundPtr->GetPanningPosition();
			if (panningPosition != 0.0)
			{
				Panner panner;
				panner.SetPosition(panningPosition);
				panner.ProcessSamples(m_tempSamples.data(), actualOut, m_tempSamples.data(), actualOut);
			}

			float v = s.second.Volume * s.second.FadeVolume;
			for (int32_t i = 0; i < actualOut; i++)
			{
				if (s.second.FadeGradient != 0.0f)
				{
					v = s.second.Volume * s.second.FadeVolume;
					s.second.FadeVolume += s.second.FadeGradient;

					if (s.second.FadeGradient > 0 && s.second.FadeVolume >= s.second.TargetedFadeVolume)
					{
						s.second.FadeGradient = 0;
						s.second.FadeVolume = s.second.TargetedFadeVolume;
					}

					if (s.second.FadeGradient < 0 && s.second.FadeVolume <= s.second.TargetedFadeVolume)
					{
						s.second.FadeGradient = 0;
						s.second.FadeVolume = s.second.TargetedFadeVolume;
					}
				}

				auto l = (int32_t) samples[i].Left + (int32_t) (m_tempSamples[i].Left * v);
				auto r = (int32_t) samples[i].Right + (int32_t) (m_tempSamples[i].Right * v);

				samples[i].Left = Clamp(l, 32767, -32768);
				samples[i].Right = Clamp(r, 32767, -32768);
			}

			minActualOut = std::min(minActualOut, actualOut);
		}

		// 削除処理
		{
			for (auto& s : m_soundStates)
			{
				if (s.second.SamplePos >= s.second.SoundPtr->GetSampleCount())
				{
					m_tempIDs.push_back(s.first);
				}
			}

			for (auto id : m_tempIDs)
			{
				m_soundStates.erase(id);
			}

			m_tempIDs.clear();
		}

		m_current += minActualOut;

		return minActualOut;
	}

	Manager_Impl::Manager_Impl()
		: m_current(0)
		, m_stateID(0)
	{
	}

	Manager_Impl::~Manager_Impl()
	{
		StopAll();
	}

	bool Manager_Impl::Initialize()
	{
		return InitializeInternal();
	}

	void Manager_Impl::Finalize()
	{
		FinalizeInternal();
		StopAll();
	}

	Sound* Manager_Impl::CreateSound(void* data, int32_t size, bool isDecompressed)
	{
		auto sound = new Sound_Impl();
		if(!sound->Load( data, size, isDecompressed))
		{
			SafeDelete(sound);
			return nullptr;
		}

		return sound;
	}

	int32_t Manager_Impl::Play( Sound* sound )
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());
		m_stateID++;

		SoundState s;
		s.SamplePos = 0;
		s.IsPaused = false;
		s.Volume = 1.0f;
		s.SoundPtr = (Sound_Impl*) sound;

		s.FadeGradient = 0.0f;
		s.FadeVolume = 1.0f;

		SafeAddRef(s.SoundPtr);
		m_soundStates[m_stateID] = s;

		return m_stateID;
	}

	bool Manager_Impl::IsPlaying(int32_t id)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());
		auto s = m_soundStates.find(id);
		return s != m_soundStates.end();
	}
		
	void Manager_Impl::StopAll()
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		for (auto s : m_soundStates)
		{
			SafeRelease(s.second.SoundPtr);
		}
		m_soundStates.clear();
	}

	void Manager_Impl::Stop(int32_t id)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				SafeRelease(s->second.SoundPtr);
				m_soundStates.erase(id);
			}
		}
	}

	void Manager_Impl::Pause(int32_t id)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				s->second.IsPaused = true;
			}
		}
	}

	void Manager_Impl::Resume(int32_t id)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				s->second.IsPaused = false;
			}
		}
	}

	void Manager_Impl::SetVolume(int32_t id, float volume)
	{
		if (volume < 0.0f) volume = 0.0f;

		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				s->second.Volume = volume;
			}
		}
	}

	void Manager_Impl::FadeIn(int32_t id, float second)
	{
		Fade(id, second, 1.0f);
	}

	void Manager_Impl::FadeOut(int32_t id, float second)
	{
		Fade(id, second, 0.0f);
	}

	void Manager_Impl::Fade(int32_t id, float second, float targetedVolume)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				auto diff = targetedVolume - s->second.FadeVolume;

				s->second.TargetedFadeVolume = targetedVolume;

				if (diff == 0.0f)
				{
					s->second.FadeGradient = 0.0f;
				}
				else if (second <= 0.0f)
				{
					if (diff > 0.0f)
					{
						s->second.FadeGradient = 10.0f;
					}
					else
					{
						s->second.FadeGradient = -10.0f;
					}
				}
				else
				{
					s->second.FadeGradient = (diff) / 44100.0f / second;
				}
			}
		}
	}
}
