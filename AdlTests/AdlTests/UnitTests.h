/*
		2011 Takahiro Harada
*/

#include <AdlPrimitives/Scan/PrefixScan.h>
#include <AdlPrimitives/Sort/RadixSort32.h>


#define NUM_TESTS 10

int g_nPassed = 0;
int g_nFailed = 0;


#define TEST_INIT bool g_testFailed = 0;
#define TEST_ASSERT(x) if( !(x) ){g_testFailed = 1;}
//#define TEST_ASSERT(x) if( !(x) ){g_testFailed = 1;ADLASSERT(x);}
#define TEST_REPORT(device, testName) \
	if(device->m_type==TYPE_CL) printf("[%s] %s(CL)\n",(g_testFailed)?"X":"O", testName); \
	else if(device->m_type==TYPE_DX11) printf("[%s] %s(DX11)\n",(g_testFailed)?"X":"O", testName); \
	else if(device->m_type==TYPE_HOST) printf("[%s] %s(Host)\n",(g_testFailed)?"X":"O", testName); \
	if(g_testFailed) g_nFailed++; else g_nPassed++;

template<DeviceType type>
void radixSort32Test( Device* deviceGPU, Device* deviceHost )
{
	TEST_INIT;
	ADLASSERT( type == deviceGPU->m_type );

	int maxSize = 1024*256;

//maxSize = 256*1024*8*2;

	HostBuffer<u32> buf0( deviceHost, maxSize );
	HostBuffer<u32> buf1( deviceHost, maxSize );
	Buffer<u32> buf2( deviceGPU, maxSize );

	RadixSort32<TYPE_HOST>::Data* dataH = RadixSort32<TYPE_HOST>::allocate( deviceHost, maxSize );
	RadixSort32<type>::Data* dataC = RadixSort32<type>::allocate( deviceGPU, maxSize );

	int dx = maxSize/NUM_TESTS;
	for(int iter=0; iter<NUM_TESTS; iter++)
	{
		int size = min2( 128+dx*iter, maxSize-512 );
		size = NEXTMULTIPLEOF( size, 512 );
//		size = 127;
//size = 256*1024*8*2;
		for(int i=0; i<size; i++) buf0[i] = getRandom(0u,0xfu);
//		for(int i=0; i<size; i++) buf0[i] = getRandom(0u,0xffffffffu);
		buf2.write( buf0.m_ptr, size );
		DeviceUtils::waitForCompletion( deviceGPU );

		RadixSort32<TYPE_HOST>::execute( dataH, buf0, size, 32 );
		RadixSort32<type>::execute( dataC, buf2, size, 4 );

		buf2.read( buf1.m_ptr, size );
		DeviceUtils::waitForCompletion( deviceGPU );
//		for(int i=0; i<size-1; i++) TEST_ASSERT( buf1[i] <= buf1[i+1] );
		for(int i=0; i<size; i++) TEST_ASSERT( buf0[i] == buf1[i] );

//		exit(0);
	}

	RadixSort32<TYPE_HOST>::deallocate( dataH );
	RadixSort32<type>::deallocate( dataC );

	TEST_REPORT( deviceGPU, "RadixSort32Test" );
}



#if defined(ADL_ENABLE_DX11)
	#if defined(ADL_ENABLE_CL)
		#define RUN_GPU( func ) func(ddcl); func(dddx);
		#define RUN_GPU_TEMPLATE( func ) func<TYPE_CL>( ddcl, ddhost ); func<TYPE_DX11>( dddx, ddhost ); 
	#else
		#define RUN_GPU( func ) func(dddx);
		#define RUN_GPU_TEMPLATE( func ) func<TYPE_DX11>( dddx, ddhost ); 
	#endif
#else
	#if defined(ADL_ENABLE_CL)
		#define RUN_GPU( func ) func(ddcl);
		#define RUN_GPU_TEMPLATE( func ) func<TYPE_CL>( ddcl, ddhost ); 
	#else
		#define RUN_GPU( func ) 
		#define RUN_GPU_TEMPLATE( func ) 
	#endif
#endif
#define RUN_ALL( func ) RUN_GPU( func ); func(ddhost);

void runAllTest()
{
	g_nPassed = 0;
	g_nFailed = 0;

	Device* ddcl;
	Device* ddhost;
#if defined(ADL_ENABLE_DX11)
	Device* dddx;
#endif

	{
		DeviceUtils::Config cfg;
//		cfg.m_type = DeviceUtils::Config::DEVICE_CPU;
#if defined(ADL_ENABLE_CL)
		ddcl = DeviceUtils::allocate( TYPE_CL, cfg );
#endif
		ddhost = DeviceUtils::allocate( TYPE_HOST, cfg );
//		cfg.m_type = DeviceUtils::Config::DEVICE_GPU;
#if defined(ADL_ENABLE_DX11)
		dddx = DeviceUtils::allocate( TYPE_DX11, cfg );
#endif
	}

	{
		char name[128];
#if defined(ADL_ENABLE_CL)
		ddcl->getDeviceName( name );
		printf("CL: %s\n", name);
#endif
#if defined(ADL_ENABLE_DX11)
		dddx->getDeviceName( name );
		printf("DX11: %s\n", name);
#endif
	}

	RUN_GPU_TEMPLATE( radixSort32Test );

#if defined(ADL_ENABLE_CL)
	DeviceUtils::deallocate( ddcl );
#endif
	DeviceUtils::deallocate( ddhost );
#if defined(ADL_ENABLE_DX11)
	DeviceUtils::deallocate( dddx );
#endif

	printf("=========\n%d Passed\n%d Failed\n", g_nPassed, g_nFailed);
}
