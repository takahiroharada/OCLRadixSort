/*
		2013 Takahiro Harada
*/
#pragma once

#include <Tahoe/Math/Math.h>

namespace Tahoe
{
struct SortData
{
	union
	{
		u32 m_key;
		struct { u16 m_key16[2]; };
	};
	u32 m_value;

	SortData(){}
	SortData(u32 key, u32 value) : m_key(key), m_value(value){}


	friend bool operator <(const SortData& a, const SortData& b)
	{
		return a.m_key < b.m_key;
	}
};


class RadixSort
{
	public:
		static
		void sort(SortData* src, int n);

		static
		void sort(u32* src, int n);

		enum
		{
			BITS_PER_PASS = 8, 
			NUM_TABLES = (1<<BITS_PER_PASS),
		};
};


};
