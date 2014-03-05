
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
			auto size = s.second.SoundPtr->GetSamples(m_tempSamples.data(), s.second.SamplePos, sampleCount);
			for (int32_t i = 0; i < size; i++)
			{
				auto l = (int32_t) samples[i].Left + (int32_t) m_tempSamples[i].Left;
				auto r = (int32_t) samples[i].Right + (int32_t) m_tempSamples[i].Right;

				samples[i].Left = Clamp(l, 32767, -32768);
				samples[i].Right = Clamp(r, 32767, -32768);
			}
			s.second.SamplePos += sampleCount;
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
		s.SoundPtr = (Sound_Impl*) sound;

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
}