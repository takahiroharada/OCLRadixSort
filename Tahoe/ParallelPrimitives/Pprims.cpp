/*
		2013 Takahiro Harada
*/
#include <Tahoe/ParallelPrimitives/Pprims.h>
#include <Tahoe/Algorithm/Sort/RadixSort.h>

using namespace adl;

namespace Tahoe
{
typedef Launcher::BufferInfo BufferInfo;

Pprims::Pprims()
{
	m_u32WorkBuffer[0] = new uArray<u32>( 1 );
	m_u32WorkBuffer[1] = new uArray<u32>( 1 );
	m_u2WorkBuffer = new uArray<uint2>( 1 );
	m_cacheKernel = true;
}

Pprims::~Pprims()
{
	if( m_u32WorkBuffer[0] ) 
		delete m_u32WorkBuffer[0];
	if( m_u32WorkBuffer[1] )
		delete m_u32WorkBuffer[1];

	if( m_u2WorkBuffer )
		delete m_u2WorkBuffer;
}
/*
void Pprims::copy( const adl::Device* device, uArray<int>& dst, const uArray<int>& src, int n )
{
	if( device == 0 )
	{
		for(int i=0; i<n; i++)
			dst[i] = src[i];
	}
	else
	{
		Launcher launcher( device, 
			device->getKernel("../Tahoe/ClKernels/PprimsKernels", "CopyIntKernel", 0, 0, 0, m_cacheKernel ) );
		dst.setToLauncher( launcher );
		src.setToLauncher( launcher );
		launcher.setConst( n );
		launcher.launch1D( n );
	}
}

void Pprims::copy( const adl::Device* device, uArray<float4>& dst, const uArray<float4>& src, int n )
{
	if( device == 0 )
	{
		for(int i=0; i<n; i++)
			dst[i] = src[i];
	}
	else
	{
		Launcher launcher( device, 
			device->getKernel("../Tahoe/ClKernels/PprimsKernels", "CopyF4Kernel", 0, 0, 0, m_cacheKernel ) );
		dst.setToLauncher( launcher );
		src.setToLauncher( launcher );
		launcher.setConst( n );
		launcher.launch1D( n );
	}
}

void Pprims::fill( const adl::Device* device, uArray<int>& dst, int src, int n )
{
	if( device == 0 )
	{
		for(int i=0; i<n; i++)
			dst[i] = src;
	}
	else
	{
		Launcher launcher( device, 
			device->getKernel("../Tahoe/ClKernels/PprimsKernels", "FillIntKernel", 0, 0, 0, m_cacheKernel ) );
		dst.setToLauncher( launcher );
		launcher.setConst( src );
		launcher.setConst( n );
		launcher.launch1D( n );
	}
}

void Pprims::fill( const adl::Device* device, uArray<u32>& dst, u32 src, int n )
{
	if( device == 0 )
	{
		for(int i=0; i<n; i++)
			dst[i] = src;
	}
	else
	{
		Launcher launcher( device, 
			device->getKernel("../Tahoe/ClKernels/PprimsKernels", "FillU32Kernel", 0, 0, 0, m_cacheKernel ) );
		dst.setToLauncher( launcher );
		launcher.setConst( src );
		launcher.setConst( n );
		launcher.launch1D( n );
	}
}

void Pprims::fill( const adl::Device* device, uArray<float4>& dst, const float4& src, int n )
{
	if( device == 0 )
	{
		for(int i=0; i<n; i++)
			dst[i] = src;
	}
	else
	{
		Launcher launcher( device, 
			device->getKernel("../Tahoe/ClKernels/PprimsKernels", "FillF4Kernel", 0, 0, 0, m_cacheKernel ) );
		dst.setToLauncher( launcher );
		launcher.setConst( src );
		launcher.setConst( n );
		launcher.launch1D( n );
	}
}
*/
void Pprims::scan( const adl::Device* device, adl::Buffer<int>& dst, const adl::Buffer<int>& src, int n, u32* sumOut )
{
	if( device == 0 )
	{
		ADLASSERT(0);
	}
	else
	{
		m_u32WorkBuffer[0]->setSize( NEXTMULTIPLEOF( max2( n/SCAN_BLOCK_SIZE, (int)SCAN_BLOCK_SIZE ), SCAN_BLOCK_SIZE ) + 1 );

		const u32 numBlocks = u32( (n+SCAN_BLOCK_SIZE*2-1)/(SCAN_BLOCK_SIZE*2) );

		if( numBlocks >= 4096 )
		{	//	todo. this is because of the implementation of TopLevelScanKernel
			TH_LOG_ERROR("Max # of elements has to be less than %d\n", 4096*2*SCAN_BLOCK_SIZE);
			return;
		}

		int4 cb;
		cb.x = n;
		cb.y = numBlocks;
		cb.z = (int)nextPowerOf2( numBlocks );

		{
			BufferInfo bInfo[] = { BufferInfo( &dst ), BufferInfo( &src ), BufferInfo( m_u32WorkBuffer[0]->getGpuBuffer(device) ) };
			Launcher launcher( device, 
				device->getKernel("../Tahoe/ClKernels/PrefixScanKernels", "LocalScanKernel", 0, 0, 0, m_cacheKernel ) );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( n );
			launcher.launch1D( numBlocks*SCAN_BLOCK_SIZE, SCAN_BLOCK_SIZE );
		}

		{
			BufferInfo bInfo[] = { BufferInfo( m_u32WorkBuffer[0]->getGpuBuffer(device) ) };
			Launcher launcher( device, 
				device->getKernel("../Tahoe/ClKernels/PrefixScanKernels", "TopLevelScanKernel", 0, 0, 0, m_cacheKernel ) );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( numBlocks );
            launcher.setConst( cb.z );
			launcher.launch1D( SCAN_BLOCK_SIZE, SCAN_BLOCK_SIZE );
		}

		if( sumOut )
		{
			((Buffer<u32>*)m_u32WorkBuffer[0]->getGpuBuffer( device ))->read( sumOut, 1, numBlocks );
		}

		if( numBlocks > 1 )
		{
			BufferInfo bInfo[] = { BufferInfo( &dst ), BufferInfo( m_u32WorkBuffer[0]->getGpuBuffer(device) ) };
			Launcher launcher( device, 
				device->getKernel("../Tahoe/ClKernels/PrefixScanKernels", "AddOffsetKernel", 0, 0, 0, m_cacheKernel ) );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( cb.x );
			launcher.launch1D( (numBlocks-1)*SCAN_BLOCK_SIZE, SCAN_BLOCK_SIZE );
		}
	}
}

struct ConstData
{
	int m_n;
	int m_nWGs;
	int m_startBit;
	int m_nBlocksPerWG;
};

__inline
bool enableSortOnDevice( const adl::Device* device )
{
	bool useGpuSort = 0;
	if( device )
	{//	todo. check this GPU is AMD GPU or not
		useGpuSort = (device->getType() == TYPE_CL && device->getProcType() == Device::Config::DEVICE_GPU);
	}
	return useGpuSort;
}

void Pprims::radixSort( const adl::Device* device, const Buffer<uint2>& inout, int n, int sortBits )
{
	if( !enableSortOnDevice( device ) )
	{
		ADLASSERT( sortBits == 32 );
		uint2* host = inout.getHostPtr( n );
		DeviceUtils::waitForCompletion( device );

		RadixSort::sort( (SortData*)host, n );

		inout.returnHostPtr( host );
		DeviceUtils::waitForCompletion( device );
	}
	else
	{
		//	todo. Find best multiplier
		//	on Cayman and Cypress, multiply 6
		//	on SI, what is the best multiplier? 4 or 8? It should be multiple of 4 as a CU has 4 SIMDs
		//	todo. On Hawaii, it's clamped. Implement PrefixScan32PerWiKernel for larger nPerWI. 
		const int NUM_WGS = min2( 255, DeviceUtils::getNCUs( device )*8 );

//		ADLASSERT( n%R32SORT_DATA_ALIGNMENT == 0 );
		ADLASSERT( R32SORT_BITS_PER_PASS == 4 );
		ADLASSERT( R32SORT_WG_SIZE == 64 );
		ADLASSERT( (sortBits&0x3) == 0 );

		if( m_u32WorkBuffer[0]->getSize() < n*2 )
			m_u32WorkBuffer[0]->setSize( n*2 );
		if( m_u32WorkBuffer[1]->getSize() < NUM_WGS*(1<<R32SORT_BITS_PER_PASS ) )
			m_u32WorkBuffer[1]->setSize( NUM_WGS*(1<<R32SORT_BITS_PER_PASS) );

		Buffer<SortData>* src = (Buffer<SortData>*)&inout;
		Buffer<SortData>* dst = (Buffer<SortData>*)m_u32WorkBuffer[0]->getGpuBuffer( device );
		Buffer<u32>* histogramBuffer = (Buffer<u32>*)m_u32WorkBuffer[1]->getGpuBuffer( device );


		int nWGs = NUM_WGS;
		ConstData cdata;
		{
			int nBlocks = (n+R32SORT_ELEMENTS_PER_WORK_ITEM*R32SORT_WG_SIZE-1)/(R32SORT_ELEMENTS_PER_WORK_ITEM*R32SORT_WG_SIZE);

			cdata.m_n = n;
			cdata.m_nWGs = NUM_WGS;
			cdata.m_startBit = 0;
			cdata.m_nBlocksPerWG = (nBlocks + cdata.m_nWGs - 1)/cdata.m_nWGs;

			if( nBlocks < NUM_WGS )
			{
				cdata.m_nBlocksPerWG = 1;
				nWGs = nBlocks;
			}
		}

		for(int ib=0; ib<sortBits; ib+=4)
		{
			cdata.m_startBit = ib;
			{
				BufferInfo bInfo[] = { BufferInfo( src, true ), BufferInfo( histogramBuffer ) };
				Launcher launcher( device, device->getKernel("../Tahoe/ClKernels/RadixSortKeyValueKernels", "StreamCountKeyValueKernel", 0, 0, 0, m_cacheKernel ) );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( cdata.m_n );
				launcher.setConst( cdata.m_nWGs );
				launcher.setConst( cdata.m_startBit );
				launcher.setConst( cdata.m_nBlocksPerWG );
				launcher.launch1D( NUM_WGS*R32SORT_WG_SIZE, R32SORT_WG_SIZE );
			}
			{//	prefix scan group histogram
				int nPerWI = ((16*NUM_WGS)+127)/128;
				Kernel* kernel;
				if( nPerWI <= 16 )
					kernel = device->getKernel("../Tahoe/ClKernels/RadixSort32Kernels", "PrefixScan16PerWiKernel", 0, 0, 0, m_cacheKernel );
				else if( nPerWI <= 32 )
					kernel = device->getKernel("../Tahoe/ClKernels/RadixSort32Kernels", "PrefixScan32PerWiKernel", 0, 0, 0, m_cacheKernel );
				else
					ADLASSERT(0);

				BufferInfo bInfo[] = { BufferInfo( histogramBuffer ) };
				Launcher launcher( device, kernel );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( cdata.m_nWGs );
				launcher.launch1D( 128, 128 );
			}

			{//	local sort and distribute
				BufferInfo bInfo[] = { BufferInfo( src, true ), BufferInfo( histogramBuffer, true ), BufferInfo( dst ) };
				Launcher launcher( device, device->getKernel("../Tahoe/ClKernels/RadixSortKeyValueKernels", "SortAndScatterKeyValueKernel", 0, 0, 0, m_cacheKernel ) );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( cdata );
/*				launcher.setConst( cdata.m_n );
				launcher.setConst( cdata.m_nWGs );
				launcher.setConst( cdata.m_startBit );
				launcher.setConst( cdata.m_nBlocksPerWG );
*/				launcher.launch1D( nWGs*R32SORT_WG_SIZE, R32SORT_WG_SIZE );
			}
			swap2( src, dst );
		}

		if( src != (Buffer<SortData>*)&inout )
		{
			dst->write( *src, n );
		}
	}
}

void Pprims::radixSort( const adl::Device* device, const Buffer<u32>& inout, int n, int sortBits )
{
	if( !enableSortOnDevice( device ) )
	{
		ADLASSERT( sortBits == 32 );
		u32* host = inout.getHostPtr( n );
		DeviceUtils::waitForCompletion( device );

		RadixSort::sort( host, n );

		inout.returnHostPtr( host );
		DeviceUtils::waitForCompletion( device );
	}
	else
	{
		//	todo. Find best multiplier
		//	on Cayman and Cypress, multiply 6
		//	on SI, what is the best multiplier? 4 or 8? It should be multiple of 4 as a CU has 4 SIMDs
		//	todo. On Hawaii, it's clamped. Implement PrefixScan32PerWiKernel for larger nPerWI. 
		const int NUM_WGS = min2( 255, DeviceUtils::getNCUs( device )*8 );

		typedef Launcher::BufferInfo BufferInfo;

		ADLASSERT( n%R32SORT_DATA_ALIGNMENT == 0 );
		ADLASSERT( R32SORT_BITS_PER_PASS == 4 );
		ADLASSERT( R32SORT_WG_SIZE == 64 );
		ADLASSERT( (sortBits&0x3) == 0 );

		m_u32WorkBuffer[0]->setSize( n );
		m_u32WorkBuffer[1]->setSize( NUM_WGS*(1<<R32SORT_BITS_PER_PASS) );

		Buffer<u32>* src = (Buffer<u32>*)&inout;
		Buffer<u32>* dst = (Buffer<u32>*)m_u32WorkBuffer[0]->getGpuBuffer( device );
		Buffer<u32>* histogramBuffer = (Buffer<u32>*)m_u32WorkBuffer[1]->getGpuBuffer( device );


		int nWGs = NUM_WGS;
		ConstData cdata;
		{
			int nBlocks = (n+R32SORT_ELEMENTS_PER_WORK_ITEM*R32SORT_WG_SIZE-1)/(R32SORT_ELEMENTS_PER_WORK_ITEM*R32SORT_WG_SIZE);

			cdata.m_n = n;
			cdata.m_nWGs = NUM_WGS;
			cdata.m_startBit = 0;
			cdata.m_nBlocksPerWG = (nBlocks + cdata.m_nWGs - 1)/cdata.m_nWGs;

			if( nBlocks < NUM_WGS )
			{
				cdata.m_nBlocksPerWG = 1;
				nWGs = nBlocks;
			}
		}

		for(int ib=0; ib<sortBits; ib+=4)
		{
			cdata.m_startBit = ib;
			{
				BufferInfo bInfo[] = { BufferInfo( src, true ), BufferInfo( histogramBuffer ) };
				Launcher launcher( device, device->getKernel("../Tahoe/ClKernels/RadixSort32Kernels", "StreamCountKernel", 0, 0, 0, m_cacheKernel ) );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( cdata.m_n );
				launcher.setConst( cdata.m_nWGs );
				launcher.setConst( cdata.m_startBit );
				launcher.setConst( cdata.m_nBlocksPerWG );
				launcher.launch1D( NUM_WGS*R32SORT_WG_SIZE, R32SORT_WG_SIZE );
			}
			{//	prefix scan group histogram
				int nPerWI = ((16*NUM_WGS)+127)/128;
				Kernel* kernel;
				if( nPerWI <= 16 )
					kernel = device->getKernel("../Tahoe/ClKernels/RadixSort32Kernels", "PrefixScan16PerWiKernel", 0, 0, 0, m_cacheKernel );
				else if( nPerWI <= 32 )
					kernel = device->getKernel("../Tahoe/ClKernels/RadixSort32Kernels", "PrefixScan32PerWiKernel", 0, 0, 0, m_cacheKernel );
				else
					ADLASSERT(0);

				BufferInfo bInfo[] = { BufferInfo( histogramBuffer ) };
				Launcher launcher( device, kernel );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( cdata.m_nWGs );
				launcher.launch1D( 128, 128 );
			}

			{//	local sort and distribute
				BufferInfo bInfo[] = { BufferInfo( src, true ), BufferInfo( histogramBuffer, true ), BufferInfo( dst ) };
				Launcher launcher( device, device->getKernel("../Tahoe/ClKernels/RadixSort32Kernels", "SortAndScatterKernel", 0, 0, 0, m_cacheKernel ) );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( cdata.m_n );
				launcher.setConst( cdata.m_nWGs );
				launcher.setConst( cdata.m_startBit );
				launcher.setConst( cdata.m_nBlocksPerWG );
				launcher.launch1D( nWGs*R32SORT_WG_SIZE, R32SORT_WG_SIZE );
			}
			swap2( src, dst );
		}

		if( src != &inout )
		{
			dst->write( *src, n );
		}
	}

}
};
