/*
		2013 Takahiro Harada
*/
#include <gtest/gtest.h>
#include <algorithm>

class DemoBase
{
	public:
		virtual
		~DemoBase(){}
};

class ParallelPrimitiveDemo : public DemoBase
{
	public:
		enum Type
		{
			TYPE_RADIX_SORT_32,
			TYPE_RADIX_SORT_KEY_VALUE,
			TYPE_SCAN, 
		};

		static
		DemoBase* createSort32() { return new ParallelPrimitiveDemo(TYPE_RADIX_SORT_32); }

		static
		DemoBase* createSortKeyValue() { return new ParallelPrimitiveDemo(TYPE_RADIX_SORT_KEY_VALUE); }

		static
		DemoBase* createScan() { return new ParallelPrimitiveDemo(TYPE_SCAN); }

		ParallelPrimitiveDemo(Type type);
		~ParallelPrimitiveDemo(){}

	public:
		const Type m_type;
};

TEST(Demo, Sort32)
{
	DemoBase* demo = ParallelPrimitiveDemo::createSort32();
	delete demo;
}

TEST(Demo, SortKeyValue)
{
	DemoBase* demo = ParallelPrimitiveDemo::createSortKeyValue();
	delete demo;
}

TEST(Demo, Scan)
{
	DemoBase* demo = ParallelPrimitiveDemo::createScan();
	delete demo;
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}

//===

#include <Adl/Adl.h>
#include <Tahoe/ParallelPrimitives/Pprims.h>
#include <Tahoe/Algorithm/Sort/RadixSort.h>

using namespace adl;
using namespace Tahoe;

char adl::s_cacheDirectory[128] = "cache";

inline 
void seedRandom(u32 i){ srand( i ); }

template<typename T>
inline
T getRandom(const T& minV, const T& maxV)
{
	double r = min((double)RAND_MAX-1, (double)rand())/RAND_MAX;
	T range = maxV - minV;
	return (T)(minV + r*range);
}

ParallelPrimitiveDemo::ParallelPrimitiveDemo(Type type) : DemoBase(), m_type(type)
{
	DeviceUtils::Config cfg;
	{
		cfg.m_type = DeviceUtils::Config::DEVICE_GPU;
//		if( ApiDemoBase::s_defaultCLDevice == Api::TYPE_ACC_CPU )
//			cfg.m_type = DeviceUtils::Config::DEVICE_CPU;
	}

	Device* d;
	d = DeviceUtils::allocate( TYPE_CL, cfg );

	{
		Pprims p;

		const int testSize = 1024*1024;

		for(int testSize = 1024; testSize<2*1024*1024; testSize *= 2 )
		{
			printf("test %6.1fK elems\n", testSize/1024.f);

			seedRandom( 123 );

			switch( m_type )
			{
			case TYPE_RADIX_SORT_32:
				{
					Buffer<u32> gpu( d, testSize );
					Array<u32> cpu( testSize );
					{
						u32* h = gpu.getHostPtr( testSize );
						DeviceUtils::waitForCompletion( d );
						for(int i=0; i<testSize; i++)
						{
							h[i] = cpu[i] = getRandom( 0u, 0xffffffff );
						}
						gpu.returnHostPtr( h );
						DeviceUtils::waitForCompletion( d );
					}
					p.radixSort( d, gpu, testSize );
					RadixSort::sort( cpu.begin(), testSize );

					{
						u32* h = gpu.getHostPtr( testSize );
						DeviceUtils::waitForCompletion( d );
						for(int i=0; i<testSize; i++)
						{
							ADLASSERT( h[i] == cpu[i] );
						}
						gpu.returnHostPtr( h );
						DeviceUtils::waitForCompletion( d );
					}
				}
				break;
			case TYPE_RADIX_SORT_KEY_VALUE:
				{
					testSize += 13;
					Buffer<SortData> gpu( d, testSize );
					uArray<SortData> cpu( testSize );
					{
						SortData* h = gpu.getHostPtr( testSize );
						DeviceUtils::waitForCompletion( d );
						for(int i=0; i<testSize; i++)
						{
							h[i] = cpu[i] = SortData( getRandom( 0u, 0xffffffff ), i );
						}
						gpu.returnHostPtr( h );
						DeviceUtils::waitForCompletion( d );
					}
					p.radixSort( d, *(Buffer<uint2>*)&gpu, testSize );
					RadixSort::sort( cpu.begin(), testSize );

					{
						SortData* h = gpu.getHostPtr( testSize );
						DeviceUtils::waitForCompletion( d );
						for(int i=0; i<testSize; i++)
						{
							ADLASSERT( h[i].m_key == cpu[i].m_key );
							ADLASSERT( h[i].m_value == cpu[i].m_value );
						}
						gpu.returnHostPtr( h );
						DeviceUtils::waitForCompletion( d );
					}
				}
				break;
			case TYPE_SCAN:
				{
					Buffer<int> gpu( d, testSize );
					Buffer<int> gpuRes( d, testSize );
					Array<int> cpu( testSize );
					{
						int* h = gpu.getHostPtr( testSize );
						DeviceUtils::waitForCompletion( d );
						for(int i=0; i<testSize; i++)
						{
							h[i] = cpu[i] = getRandom( 0, 0xf );
						}
						gpu.returnHostPtr( h );
						DeviceUtils::waitForCompletion( d );
					}
					p.scan( d, gpuRes, gpu, testSize );

					{
						int* h = gpuRes.getHostPtr( testSize );
						DeviceUtils::waitForCompletion( d );
						int ans = 0;
						bool fail = 0;
						for(int i=0; i<testSize; i++)
						{
							fail |= ( h[i] != ans );
							ans += cpu[i];
						}
						ADLASSERT( fail == 0 );
						gpuRes.returnHostPtr( h );
						DeviceUtils::waitForCompletion( d );
					}
				}
				break;
			default:
				break;
			};
		}
	}

	DeviceUtils::deallocate( d );
}
