
#include "osm.Manager_Impl.h"
#include "osm.Sound_Impl.h"

namespace osm
{
	int32_t Manager_Impl::ReadSamples(Sample* samples, int32_t sampleCount)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		// ‡¬ˆ—
		m_tempSamples.resize(sampleCount);

		memset(samples, 0, sizeof(Sample) * sampleCount);

		for (auto& s : m_soundStates)
		{
			if (s.second.IsPaused) continue;

			float v = s.second.Volume * s.second.FadeVolume;

			int32_t rest = sampleCount;
			auto loopStart = (int32_t)(s.second.SoundPtr->GetLoopStartingPoint() * 44100);
			auto loopEnd = (int32_t) (s.second.SoundPtr->GetLoopEndPoint() * 44100);
			auto enabledLoop = s.second.SoundPtr->GetIsLoopingMode();

			if (loopEnd > s.second.SoundPtr->GetSampleCount()) loopEnd = s.second.SoundPtr->GetSampleCount();
			if (loopStart == loopEnd) enabledLoop = false;
			if (loopStart > loopEnd) enabledLoop = false;
			if (loopEnd < 0) enabledLoop = false;

			while (rest > 0)
			{
				int32_t readSize = sampleCount;

				// loop
				if (enabledLoop)
				{
					if (s.second.SamplePos + sampleCount >= loopEnd)
					{
						readSize = loopEnd - s.second.SamplePos;
					}
				}

				auto size = s.second.SoundPtr->GetSamples(m_tempSamples.data(), s.second.SamplePos, readSize);
				for (int32_t i = 0; i < size; i++)
				{
					if (s.second.FadeGradient != 0.0f)
					{
						v = s.second.Volume * s.second.FadeVolume;
						s.second.FadeVolume += s.second.FadeGradient;

						if (s.second.FadeGradient > 0 && s.second.FadeVolume >= 1.0f)
						{
							s.second.FadeGradient = 0;
							s.second.Volume = 1.0f;
						}

						if (s.second.FadeGradient < 0 && s.second.FadeVolume <= 0.0f)
						{
							s.second.FadeGradient = 0;
							s.second.Volume = 0.0f;
						}
					}

					auto l = (int32_t) samples[i].Left + (int32_t) (m_tempSamples[i].Left * v);
					auto r = (int32_t) samples[i].Right + (int32_t) (m_tempSamples[i].Right * v);

					samples[i].Left = Clamp(l, 32767, -32768);
					samples[i].Right = Clamp(r, 32767, -32768);
				}
				s.second.SamplePos +=size;

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
			
		}

		// íœˆ—
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

		m_current += sampleCount;

		return sampleCount;
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
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				s->second.FadeVolume = 0.0f;
				s->second.FadeGradient = 1.0f / 44100.0f / second;
			}
		}
	}

	void Manager_Impl::FadeOut(int32_t id, float second)
	{
		std::lock_guard<std::recursive_mutex> lock(GetMutex());

		{
			auto s = m_soundStates.find(id);
			if (s != m_soundStates.end())
			{
				s->second.FadeGradient = - 1.0f / 44100.0f / second;
			}
		}
	}
}
