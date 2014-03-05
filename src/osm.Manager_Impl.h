
#pragma once

#include "OpenSoundMixer.h"

#include <mutex>
#include <map>
#include <vector>

namespace osm
{
	class Sound_Impl;

	class Manager_Impl
		: public Manager
	{
	private:
	
		struct SoundState
		{
			int32_t				SamplePos;
			Sound_Impl*	SoundPtr;
		};

		int32_t	m_stateID;
		int32_t	m_current;

		std::map<int32_t, SoundState>	m_soundStates;

		std::vector<Sample>			m_tempSamples;
		std::vector<int32_t>		m_tempIDs;

		std::recursive_mutex		m_mutex;

	protected:
		virtual bool InitializeInternal() = 0;
		virtual void FinalizeInternal() = 0;

		int32_t ReadSamples( Sample* samples, int32_t sampleCount );

		/**
		@brief	制御用のミューテックスを取得する。
		@return	ミューテックス
		*/
		std::recursive_mutex& GetMutex(){ return m_mutex; }
	public:
		Manager_Impl();
		virtual ~Manager_Impl();

		bool Initialize();
		void Finalize();

		Sound* CreateSound(void* data, int32_t size, bool isDecompressed);

		int32_t Play( Sound* sound );
	
		bool IsPlaying(int32_t id);

		void StopAll();

		void Stop(int32_t id);
	};
}