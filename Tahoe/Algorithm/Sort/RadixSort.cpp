/*
		2013 Takahiro Harada
*/
#include <Tahoe/Algorithm/Sort/RadixSort.h>
//#include <Tahoe/Math/Array.h>

namespace Tahoe
{

void RadixSort::sort(SortData* data, int n)
{
	SortData* workBuffer = new SortData[n];

	int tables[NUM_TABLES];
	int counter[NUM_TABLES];

	SortData* src = data;
	SortData* dst = workBuffer;

	for(int startBit=0; startBit<32; startBit+=BITS_PER_PASS)
	{
		for(int i=0; i<NUM_TABLES; i++)
		{
			tables[i] = 0;
		}

		for(int i=0; i<n; i++)
		{
			int tableIdx = (src[i].m_key >> startBit) & (NUM_TABLES-1);
			tables[tableIdx]++;
		}

		//	prefix scan
		int sum = 0;
		for(int i=0; i<NUM_TABLES; i++)
		{
			int iData = tables[i];
			tables[i] = sum;
			sum += iData;
			counter[i] = 0;
		}

		//	distribute
		for(int i=0; i<n; i++)
		{
			int tableIdx = (src[i].m_key >> startBit) & (NUM_TABLES-1);
			
			dst[tables[tableIdx] + counter[tableIdx]] = src[i];
			counter[tableIdx] ++;
		}

		swap2( src, dst );
	}

	delete [] workBuffer;
}

void RadixSort::sort(u32* data, int n)
{
	u32* workBuffer = new u32[n];

	int tables[NUM_TABLES];
	int counter[NUM_TABLES];

	u32* src = data;
	u32* dst = workBuffer;

	for(int startBit=0; startBit<32; startBit+=BITS_PER_PASS)
	{
		for(int i=0; i<NUM_TABLES; i++)
		{
			tables[i] = 0;
		}

		for(int i=0; i<n; i++)
		{
			int tableIdx = (src[i] >> startBit) & (NUM_TABLES-1);
			tables[tableIdx]++;
		}

		//	prefix scan
		int sum = 0;
		for(int i=0; i<NUM_TABLES; i++)
		{
			int iData = tables[i];
			tables[i] = sum;
			sum += iData;
			counter[i] = 0;
		}

		//	distribute
		for(int i=0; i<n; i++)
		{
			int tableIdx = (src[i] >> startBit) & (NUM_TABLES-1);
			
			dst[tables[tableIdx] + counter[tableIdx]] = src[i];
			counter[tableIdx] ++;
		}

		swap2( src, dst );
	}

	delete [] workBuffer;
}

};
