/*
		2013 Takahiro Harada
*/
#pragma once

#include <Tahoe/ParallelPrimitives/uArray.h>

namespace Tahoe
{

class Pprims
{
public:
	TH_DECLARE_ALLOCATOR(Pprims);
	
	Pprims();

	~Pprims();

	void cacheKernel( bool cache ) { m_cacheKernel = cache; }

	enum
	{
		SCAN_BLOCK_SIZE = 128,

		RSORT_BITS_PER_PASS = 8,
		RSORT_NUM_TABLES = (1<<RSORT_BITS_PER_PASS),

		R32SORT_DATA_ALIGNMENT = 256,
		R32SORT_WG_SIZE = 64,
		R32SORT_ELEMENTS_PER_WORK_ITEM = (256/R32SORT_WG_SIZE),
		R32SORT_BITS_PER_PASS = 4,
	};

	void scan( const adl::Device* device, adl::Buffer<int>& dst, const adl::Buffer<int>& src, int n, u32* sumOut = 0 );

	//	inout.x: key, inout.y: value
	void radixSort( const adl::Device* device, const adl::Buffer<uint2>& inout, int n, int sortBits = 32 );

	//	n has to be multiple of DATA_ALIGNMENT
	void radixSort( const adl::Device* device, const adl::Buffer<u32>& inout, int n, int sortBits = 32 );

private:
	uArray<u32>* m_u32WorkBuffer[2];
	uArray<uint2>* m_u2WorkBuffer;

	bool m_cacheKernel;
};


};
