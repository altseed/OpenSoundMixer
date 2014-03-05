
#pragma once

#include "OpenSoundMixer.h"

namespace osm
{
	/**
	@brief	参照カウンタオブジェクト
	*/
	class ReferenceObject
		: public IReference
	{
	private:
		mutable std::atomic<int32_t> m_reference;

	public:
		ReferenceObject();

		virtual ~ReferenceObject();

		virtual int AddRef();

		virtual int GetRef();

		virtual int Release();
	};
}