/*
		2011 Takahiro Harada
*/

#pragma comment(lib,"OpenCL.lib")
#if defined(__APPLE__)
	#include <OpenCL/cl.h>
	#include <OpenCL/cl_ext.h>
	#include <OpenCL/cl_gl_ext.h>
	#include <OpenCL/cl_platform.h>
#else
	#include <CL/cl.h>
	#include <CL/cl_ext.h>
	#include <CL/cl_gl.h>
	#include <CL/cl_platform.h>
#endif

#include <string.h>

namespace adl
{

struct DeviceCL : public Device
{
	typedef DeviceUtils::Config Config;


	__inline
	DeviceCL() : Device( TYPE_CL ), m_kernelManager(0){}
	__inline
	void* getContext() const { return m_context; }
	__inline
	void initialize(const Config& cfg);
	__inline
	void release();

	template<typename T>
	__inline
	void allocate(Buffer<T>* buf, u64 nElems, BufferBase::BufferType type);

	template<typename T>
	__inline
	void deallocate(Buffer<T>* buf);

	template<typename T>
	__inline
	void copy(Buffer<T>* dst, const Buffer<T>* src, u64 nElems, SyncObject* syncObj = 0);

	template<typename T>
	__inline
	void copy(T* dst, const Buffer<T>* src, u64 nElems, u64 srcOffsetNElems = 0, SyncObject* syncObj = 0);

	template<typename T>
	__inline
	void copy(Buffer<T>* dst, const T* src, u64 nElems, u64 dstOffsetNElems = 0, SyncObject* syncObj = 0);

    template<typename T>
	__inline
	void clear(const Buffer<T>* const src) const;

	template<typename T>
	__inline
	void fill(const Buffer<T>* const src, void* pattern, int patternSize) const;

	template<typename T>
	__inline
	void getHostPtr(const Buffer<T>* const src, u64 size, T*& ptr) const;

	template<typename T>
	__inline
	void returnHostPtr(const Buffer<T>* const src, T* ptr) const;

	__inline
	void waitForCompletion() const;

	__inline
	void waitForCompletion( const SyncObject* syncObj ) const;

	__inline
	bool isComplete( const SyncObject* syncObj ) const;
    
	__inline
	void flush() const;

	__inline
	void allocateSyncObj( void*& ptr ) const;

	__inline
	void deallocateSyncObj( void* ptr ) const;

	__inline
	void getDeviceName( char nameOut[128] ) const;

	__inline
	void getDeviceVendor( char nameOut[128] ) const;

	__inline
	cl_event* extract( SyncObject* syncObj )
	{
		if( syncObj == 0 ) return 0;
		ADLASSERT( syncObj->m_device->getType() == TYPE_CL );
		cl_event* e = (cl_event*)syncObj->m_ptr;
		if( *e )
		{
			cl_int status = clReleaseEvent( *e );
			ADLASSERT( status == CL_SUCCESS );
		}
		return e;
	}

	__inline
	static
	int getNDevices();

	__inline
	int getNCUs() const;

	__inline
	u64 getMemSize() const;

	__inline
	u64 getMaxAllocationSize() const;

	__inline
	Kernel* getKernel(const char* fileName, const char* funcName, const char* option = NULL, const char** srcList = NULL, int nSrc = 0, bool cacheKernel = true )const;


	enum
	{
		MAX_NUM_DEVICES = 32,
	};
	
	cl_context m_context;
	cl_command_queue m_commandQueue;
	cl_device_id m_deviceIdx;

	KernelManager* m_kernelManager;

	char m_deviceName[256];
	char m_driverVersion[256];
    
    int m_maxLocalWGSize[3];
};

//===
//===

void DeviceCL::initialize(const Config& cfg)
{
	debugPrintf("> CL Initialization\n");
	m_procType = cfg.m_type;
	{
		DeviceCL* deviceData = (DeviceCL*)this;

		cl_device_type deviceType = (cfg.m_type== Config::DEVICE_GPU)? CL_DEVICE_TYPE_GPU: CL_DEVICE_TYPE_CPU;
		bool enableProfiling = false;
#ifdef _DEBUG
		enableProfiling = true;
#endif
		cl_int status;

		cl_platform_id platform;
		{
			cl_uint nPlatforms = 0;
			status = clGetPlatformIDs(0, NULL, &nPlatforms);
			debugPrintf("CL %d platforms\n", nPlatforms);
			ADLASSERT( status == CL_SUCCESS );

			cl_platform_id pIdx[5];
			status = clGetPlatformIDs(nPlatforms, pIdx, NULL);
			ADLASSERT( status == CL_SUCCESS );

			cl_uint atiIdx = (cl_uint)-1;
			cl_uint intelIdx = (cl_uint)-1;
			cl_uint nvIdx = (cl_uint)-1;
			cl_uint appleIdx = (cl_uint)-1;

			for(cl_uint i=0; i<nPlatforms; i++)
			{
				char buff[512];
				status = clGetPlatformInfo( pIdx[i], CL_PLATFORM_VENDOR, 512, buff, 0 );
				debugPrintf("CL Vendors: %s\n", buff);
				ADLASSERT( status == CL_SUCCESS );

				//skip the platform if there are no devices available
				cl_uint numDevice;
				status = clGetDeviceIDs( pIdx[i], deviceType, 0, NULL, &numDevice );
				if (numDevice>0)
				{
					if( strcmp( buff, "NVIDIA Corporation" )==0 ) nvIdx = i;
					if( strcmp( buff, "Advanced Micro Devices, Inc." )==0 ) atiIdx = i;
					if( strcmp( buff, "Intel(R) Corporation" )==0 ) intelIdx = i;
					if( strcmp( buff, "Apple" )==0 ) appleIdx = i;
				}
			}
			{
				platform = (cl_platform_id)(-1);
				switch( cfg.m_vendor )
				{
				case DeviceUtils::Config::VD_AMD:
					if( atiIdx != -1 )
						platform = pIdx[atiIdx];
					break;
				case DeviceUtils::Config::VD_NV:
					if( nvIdx != -1 )
						platform = pIdx[nvIdx];
					break;
				case DeviceUtils::Config::VD_INTEL:
					if( intelIdx != -1 )
						platform = pIdx[intelIdx];
					break;
				default:
					ADLASSERT(0);
					break;
				};

				if( platform == (cl_platform_id)(-1) )
				{
					if( atiIdx != (cl_uint)-1 ) platform = pIdx[atiIdx];
					else if( nvIdx != (cl_uint)-1 ) platform = pIdx[nvIdx];
					else if( appleIdx != (cl_uint)-1 ) platform = pIdx[appleIdx];
					else if( intelIdx != (cl_uint)-1 )platform = pIdx[intelIdx];
					else ADLASSERT(0);
				}
			}
		}

		cl_uint numDevice;
		status = clGetDeviceIDs( platform, deviceType, 0, NULL, &numDevice );

//		ADLASSERT( cfg.m_deviceIdx < (int)numDevice );

		debugPrintf("CL: %d %s Devices ", (int)numDevice, (deviceType==CL_DEVICE_TYPE_GPU)? "GPU":"CPU");

		ADLASSERT( numDevice <= MAX_NUM_DEVICES );
		cl_device_id deviceIds[ MAX_NUM_DEVICES ];

		status = clGetDeviceIDs( platform, deviceType, numDevice, deviceIds, NULL );
		ADLASSERT( status == CL_SUCCESS );

		{
			deviceData->m_context = 0;

			m_deviceIdx = deviceIds[min2( (int)numDevice-1, cfg.m_deviceIdx )];

			cl_context_properties fullProps[32] = {0};
			if( cfg.m_clContextProperties )
			{	//	for gl cl interop
				cl_context_properties* props = (cl_context_properties*)cfg.m_clContextProperties;
				int i =0;
				while( props[i] )
				{
					fullProps[i] = props[i];
					i++;
				}
//#if !defined(__APPLE__)
				fullProps[i] = CL_CONTEXT_PLATFORM;
				fullProps[i+1] = (cl_context_properties)platform;
//#endif
				// if there are more than 1 gpus in a system, the device index should be the device index which gl context is created
//				for(cl_uint idx=0; idx<numDevice; idx++)


				cl_device_id glDeviceId = m_deviceIdx;

				//	this was necessary for FMP-84, to use 4GPUs in Maya plugin
#if defined(__WIN32__)
				static clGetGLContextInfoKHR_fn clGetGLContextInfoKHR = 0;
				if (!clGetGLContextInfoKHR)
				{
					clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
				}
				if( clGetGLContextInfoKHR )
				{
					status = clGetGLContextInfoKHR( fullProps, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &glDeviceId, 0 );
					ADLASSERT( status == CL_SUCCESS );
				}
#else
//                status = clGetGLContextInfoAPPLE( fullProps, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &glDeviceId, 0 );
//                ADLASSERT( status == CL_SUCCESS );
#endif
				if( glDeviceId == m_deviceIdx )
				{
					deviceData->m_context = clCreateContext( fullProps, 1, &m_deviceIdx, NULL, NULL, &status );
					if( deviceData->m_context != 0 )
						deviceData->m_interopAvailable = 1;
//						break;
//					clReleaseContext( deviceData->m_context );
				}
			}
			if( deviceData->m_context == 0 )
			{
				deviceData->m_context = clCreateContext( 0, 1, &deviceData->m_deviceIdx, NULL, NULL, &status );
			}
			ADLASSERT( status == CL_SUCCESS );

			char buff[512];
			status = clGetDeviceInfo( deviceData->m_deviceIdx, CL_DEVICE_NAME, sizeof(buff), &buff, NULL );
			ADLASSERT( status == CL_SUCCESS );

			debugPrintf("[%s]\n", buff);

			deviceData->m_commandQueue = clCreateCommandQueue( deviceData->m_context, deviceData->m_deviceIdx, (enableProfiling)?CL_QUEUE_PROFILING_ENABLE:0, NULL );

			ADLASSERT( status == CL_SUCCESS );

		//	status = clSetCommandQueueProperty( commandQueue, CL_QUEUE_PROFILING_ENABLE, CL_TRUE, 0 );
		//	CLASSERT( status == CL_SUCCESS );

			if(0)
			{
				cl_bool image_support;
				clGetDeviceInfo(deviceData->m_deviceIdx, CL_DEVICE_IMAGE_SUPPORT, sizeof(image_support), &image_support, NULL);
				debugPrintf("	CL_DEVICE_IMAGE_SUPPORT : %s\n", image_support?"Yes":"No");
			}
		}
	}

	m_kernelManager = new KernelManager;

	{
		getDeviceName( m_deviceName );
		clGetDeviceInfo( m_deviceIdx, CL_DRIVER_VERSION, 256, &m_driverVersion, NULL);
        
        
		DeviceCL* deviceData = (DeviceCL*)this;
        size_t wgSize[3];
        clGetDeviceInfo( deviceData->m_deviceIdx, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(wgSize), wgSize, 0 );
        m_maxLocalWGSize[0] = (int)wgSize[0];
        m_maxLocalWGSize[1] = (int)wgSize[1];
        m_maxLocalWGSize[2] = (int)wgSize[2];

        size_t ww;
        clGetDeviceInfo( deviceData->m_deviceIdx, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(ww), &ww, 0 );
        
        
        cl_uint maxWorkDim;
        clGetDeviceInfo( deviceData->m_deviceIdx, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(maxWorkDim), &maxWorkDim, 0 );

        debugPrintf("CL_DEVICE_MAX_WORK_ITEM_SIZES  : %d, %d, %d\n", m_maxLocalWGSize[0], m_maxLocalWGSize[1], m_maxLocalWGSize[2]);
        debugPrintf("CL_DEVICE_MAX_WORK_GROUP_SIZE  : %d\n", (int)ww);
        debugPrintf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS : %d\n", (int)maxWorkDim);
	}
	debugPrintf("< CL Initialization\n");	
}

void DeviceCL::release()
{
	ADLASSERT( m_memoryUsage == 0 );
	clReleaseCommandQueue( m_commandQueue );
	clReleaseContext( m_context );

	if( m_kernelManager ) delete m_kernelManager;
}

template<typename T>
void DeviceCL::allocate(Buffer<T>* buf, u64 nElems, BufferBase::BufferType type)
{
	buf->m_device = this;
	buf->m_size = nElems;
	buf->m_ptr = 0;

	if( type == BufferBase::BUFFER_CONST ) return;

#if defined(ADL_CL_DUMP_MEMORY_LOG)
//	char deviceName[256];
//	getDeviceName( deviceName );
//   	printf( "adlCLMemoryLog	%s : %3.2fMB	Allocation: %3.2fKB ", deviceName, m_memoryUsage/1024.f/1024.f, sizeof(T)*nElems/1024.f );
//	fflush( stdout );
#endif
	nElems = max2( (u64)1, nElems );
	cl_int status = 0;
	if( sizeof(T)*buf->m_size >= getMaxAllocationSize() )
	{
		char* ptr = new char[sizeof(T)*nElems];
		buf->m_ptr = (T*)clCreateBuffer( m_context, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, sizeof(T)*nElems, ptr, &status );
		buf->m_cl.m_hostPtr = ptr;
	}
	else
	{
	if( type == BufferBase::BUFFER_ZERO_COPY )
		buf->m_ptr = (T*)clCreateBuffer( m_context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(T)*nElems, 0, &status );
	else if( type == BufferBase::BUFFER_RAW )
		buf->m_ptr = (T*)clCreateBuffer( m_context, CL_MEM_WRITE_ONLY, sizeof(T)*nElems, 0, &status );
	else
		buf->m_ptr = (T*)clCreateBuffer( m_context, CL_MEM_READ_WRITE, sizeof(T)*nElems, 0, &status );
	}

//	printf( "clCreateBuffer: %s (%3.2fMB)	%dB\n", (status==CL_SUCCESS)? "O": "X", m_memoryUsage/1024.f/1024.f, buf->m_size*sizeof(T) );
	if( status != CL_SUCCESS )
	{
		Tahoe::TH_LOG_ERROR("CL Memory Allocation Failure: %3.2fMB, Total used memory: %3.2fMB\n", 
			buf->m_size*sizeof(T)/1024.f/1024.f, m_memoryUsage/1024.f/1024.f);

		cl_ulong size;
		clGetDeviceInfo( m_deviceIdx, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(size), &size, 0 );
		Tahoe::TH_LOG_ERROR("CL Max Memory Allocation Size: %3.2fMB\n", size/1024.f/1024.f);

//		printf("  Error %d (%3.2fMB allocation, %3.2fMB used)\n", status, buf->m_size*sizeof(T)/1024.f/1024.f, m_memoryUsage/1024.f/1024.f);
//		if( status == CL_MEM_OBJECT_ALLOCATION_FAILURE )
//			printf("	Can be out of memory");
//		exit(0);
		buf->m_size = 0;
		buf->m_ptr = 0;
		return;
	}

	m_memoryUsage += buf->m_size*sizeof(T);

#if defined(_DEBUG)
	if( status == CL_INVALID_BUFFER_SIZE )
	{
		cl_ulong size;
		clGetDeviceInfo( m_deviceIdx, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(size), &size, 0 );
		printf("CL_DEVICE_MAX_MEM_ALLOC_SIZE: %3.2f MB\n", size/1024.f/1024.f);
		printf("Trying to allocate %3.2fMB\n", sizeof(T)*nElems/sizeof(char)/1024.f/1024.f);
	}
#endif
	ADLASSERT( status == CL_SUCCESS );
}

template<typename T>
void DeviceCL::deallocate(Buffer<T>* buf)
{
	if( buf->m_ptr )
	{
		ADLASSERT( m_memoryUsage >= buf->m_size*sizeof(T) );
		m_memoryUsage -= buf->m_size*sizeof(T);
		clReleaseMemObject( (cl_mem)buf->m_ptr );
	}
	buf->m_device = 0;
	buf->m_size = 0;
	buf->m_ptr = 0;

	if( buf->m_cl.m_hostPtr )
	{
		delete [] buf->m_cl.m_hostPtr;
	}
}

template<typename T>
void DeviceCL::copy(Buffer<T>* dst, const Buffer<T>* src, u64 nElems, SyncObject* syncObj )
{
	cl_event* e = extract( syncObj );

	if( dst->m_device->m_type == TYPE_CL && src->m_device->m_type == TYPE_CL )
	{
		cl_int status = 0;
		status = clEnqueueCopyBuffer( m_commandQueue, (cl_mem)src->m_ptr, (cl_mem)dst->m_ptr, 0, 0, sizeof(T)*nElems, 0, 0, e );
		ADLASSERT( status == CL_SUCCESS );
	}
	else if( src->m_device->m_type == TYPE_HOST )
	{
		ADLASSERT( dst->getType() == TYPE_CL );
		dst->write( src->m_ptr, nElems, 0, syncObj );
	}
	else if( dst->m_device->m_type == TYPE_HOST )
	{
		ADLASSERT( src->getType() == TYPE_CL );
		src->read( dst->m_ptr, nElems, 0, syncObj );
	}
	else
	{
		ADLASSERT( 0 );
	}
}

template<typename T>
void DeviceCL::copy(T* dst, const Buffer<T>* src, u64 nElems, u64 srcOffsetNElems, SyncObject* syncObj )
{
	cl_event* e = extract( syncObj );
	cl_int status = 0;
/*	{
		void* ptr = clEnqueueMapBuffer( m_commandQueue, (cl_mem)src->m_ptr, 1, CL_MAP_READ, sizeof(T)*srcOffsetNElems, sizeof(T)*nElems, 
			0,0,0,&status );
		ADLASSERT( status == CL_SUCCESS );
		memcpy( dst, ptr, sizeof(T)*nElems );
		status = clEnqueueUnmapMemObject( m_commandQueue, (cl_mem)src->m_ptr, ptr, 0, 0, 0 );
		ADLASSERT( status == CL_SUCCESS );
	}
*/
	status = clEnqueueReadBuffer( m_commandQueue, (cl_mem)src->m_ptr, 0, sizeof(T)*srcOffsetNElems, sizeof(T)*nElems,
		dst, 0,0,e );
	ADLASSERT( status == CL_SUCCESS );
#if defined(_DEBUG)
	waitForCompletion();
#endif
}

template<typename T>
void DeviceCL::copy(Buffer<T>* dst, const T* src, u64 nElems, u64 dstOffsetNElems, SyncObject* syncObj )
{
	cl_event* e = extract( syncObj );
	cl_int status = 0;
/*	{
		void* ptr = clEnqueueMapBuffer( m_commandQueue, (cl_mem)dst->m_ptr, 1, CL_MAP_WRITE, sizeof(T)*dstOffsetNElems, sizeof(T)*nElems,
			0,0,0,&status );
		ADLASSERT( status == CL_SUCCESS );
		memcpy( ptr, src, sizeof(T)*nElems );
		status = clEnqueueUnmapMemObject( m_commandQueue, (cl_mem)dst->m_ptr, ptr, 0, 0, 0 );
		ADLASSERT( status == CL_SUCCESS );
	}
*/
	status = clEnqueueWriteBuffer( m_commandQueue, (cl_mem)dst->m_ptr, 0, sizeof(T)*dstOffsetNElems, sizeof(T)*nElems,
		src, 0,0,e );
	ADLASSERT( status == CL_SUCCESS );
#if defined(_DEBUG)
	waitForCompletion();
#endif
}

template<typename T>
void DeviceCL::clear(const Buffer<T>* const src) const
{
    const char* source =  "__kernel void memclear(__global char* mem, uint size)"
                    "{"
                    "    if (get_global_id(0) < size)"
                    "        mem[get_global_id(0)] = 0; "
                    "}";

    int size = src->getSize()*sizeof(T);

	Kernel* kernel = getKernel(0, "memclear", 0, &source, 1 );

    Launcher launcher(this, kernel);

    Launcher::BufferInfo bInfo = Launcher::BufferInfo( src );
   launcher.setBuffers(&bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo));

    launcher.setConst( size );

    launcher.launch1D( src->getSize()*sizeof(T) );

//    clFinish( m_commandQueue );
}

template<typename T>void DeviceCL::fill(const Buffer<T>* const src, void* pattern, int patternSize) const
{
	cl_int status = clEnqueueFillBuffer( m_commandQueue, (cl_mem)src->m_ptr, pattern, patternSize, 0, src->getSize()*sizeof(T), 0, 0, 0 );

	ADLASSERT( status == CL_SUCCESS );
}

template<typename T>
void DeviceCL::getHostPtr(const Buffer<T>* const src, u64 size, T*& ptr) const
{
	size = (size == -1)? src->getSize():size;
	cl_int e;
	ptr = (T*)clEnqueueMapBuffer( m_commandQueue, (cl_mem)src->m_ptr, false, CL_MAP_READ | CL_MAP_WRITE, 0, 
		sizeof(T)*size, 0,0,0,&e );
	ADLASSERT( e == CL_SUCCESS );
#if defined(_DEBUG)
	waitForCompletion();
#endif
}

template<typename T>
void DeviceCL::returnHostPtr(const Buffer<T>* const src, T* ptr) const
{
	cl_int e = clEnqueueUnmapMemObject( m_commandQueue, (cl_mem)src->m_ptr, ptr, 0,0,0 );
	ADLASSERT( e == CL_SUCCESS );
#if defined(_DEBUG)
	waitForCompletion();
#endif
}

void DeviceCL::waitForCompletion() const
{
	clFinish( m_commandQueue );
}

void DeviceCL::waitForCompletion( const SyncObject* syncObj ) const
{
	if( syncObj )
	{
		ADLASSERT( syncObj->m_device->getType() == TYPE_CL );
		cl_event* e = (cl_event*)syncObj->m_ptr;
		if( *e )
		{
			cl_int status = clWaitForEvents( 1, e );
			ADLASSERT( status == CL_SUCCESS );
			status = clReleaseEvent( *e );
			ADLASSERT( status == CL_SUCCESS );
			*e = 0;
		}
	}
}
    
bool DeviceCL::isComplete( const SyncObject* syncObj ) const
{
    if( syncObj )
    {
        ADLASSERT( syncObj->m_device->getType() == TYPE_CL );
        cl_event* e = (cl_event*)syncObj->m_ptr;
        if( *e )
        {
            int status;
            int er = clGetEventInfo( *e, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(int), (void*)&status, 0);
            ADLASSERT( er == CL_SUCCESS );
            
            if( status == CL_COMPLETE )
            {
                er = clReleaseEvent( *e );
                ADLASSERT( er == CL_SUCCESS );
                *e = 0;
                return 1;
            }
            return 0;
        }
    }
    return 1;
}

void DeviceCL::flush() const
{
	clFlush( m_commandQueue );
}

void DeviceCL::allocateSyncObj( void*& ptr ) const
{
	cl_event* e = (cl_event*)new cl_event;
	*e = 0;
	ptr = e;
}

void DeviceCL::deallocateSyncObj( void* ptr ) const
{
	cl_event* e = (cl_event*)ptr;
	if( *e )
	{
		cl_int status = clReleaseEvent( *e );
		ADLASSERT( status == CL_SUCCESS );
	}
	delete e;
}

int DeviceCL::getNDevices()
{
	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;
	cl_int status;

	cl_platform_id platform;
	int nDevices = 0;
	{
		cl_uint nPlatforms = 0;
		status = clGetPlatformIDs(0, NULL, &nPlatforms);
		ADLASSERT( status == CL_SUCCESS );

		cl_platform_id pIdx[5];
		status = clGetPlatformIDs(nPlatforms, pIdx, NULL);
		ADLASSERT( status == CL_SUCCESS );

		cl_uint atiIdx = (cl_uint)-1;
		cl_uint intelIdx = (cl_uint)-1;
		cl_uint nvIdx = (cl_uint)-1;
		cl_uint appleIdx = (cl_uint)-1;
		for(cl_uint i=0; i<nPlatforms; i++)
		{
			char buff[512];
			status = clGetPlatformInfo( pIdx[i], CL_PLATFORM_VENDOR, 512, buff, 0 );
			ADLASSERT( status == CL_SUCCESS );

			if( strcmp( buff, "NVIDIA Corporation" )==0 ) nvIdx = i;
			if( strcmp( buff, "Advanced Micro Devices, Inc." )==0 ) atiIdx = i;
			if( strcmp( buff, "Intel(R) Corporation" )==0 ) intelIdx = i;
			if( strcmp( buff, "Apple" )==0 ) appleIdx = i;
		}
		{
			cl_uint n;
			if( atiIdx != (cl_uint)-1 )
			{
				platform = pIdx[atiIdx];
				status = clGetDeviceIDs( platform, deviceType, 0, NULL, &n );
				ADLASSERT( status == CL_SUCCESS );
				nDevices += n;
			}
			if( nvIdx != (cl_uint)-1 )
			{
				platform = pIdx[nvIdx];
				status = clGetDeviceIDs( platform, deviceType, 0, NULL, &n );
				ADLASSERT( status == CL_SUCCESS );
				nDevices += n;
			}
			if( appleIdx != (cl_uint)-1 )
			{
				platform = pIdx[appleIdx];
				status = clGetDeviceIDs( platform, deviceType, 0, NULL, &n );
				ADLASSERT( status == CL_SUCCESS );
				nDevices += n;
			}
			if( intelIdx != (cl_uint)-1 )
			{
				platform = pIdx[intelIdx];
				status = clGetDeviceIDs( platform, deviceType, 0, NULL, &n );
				//ADLASSERT( status == CL_SUCCESS );
				nDevices += n;
			}
		}
	}

	return nDevices;
}

int DeviceCL::getNCUs() const
{
	unsigned int nCUs;
	clGetDeviceInfo( m_deviceIdx, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(unsigned int), &nCUs, 0 );
	return nCUs;
}

unsigned long long DeviceCL::getMemSize() const
{
	cl_ulong size;
	clGetDeviceInfo( m_deviceIdx, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(size), &size, 0 );
	return size;
}

unsigned long long DeviceCL::getMaxAllocationSize() const
{
	cl_ulong size;
	clGetDeviceInfo( m_deviceIdx, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(size), &size, 0 );
	return size;
}

void DeviceCL::getDeviceName( char nameOut[128] ) const
{
	cl_int status;
	status = clGetDeviceInfo( m_deviceIdx, CL_DEVICE_NAME, sizeof(char)*128, nameOut, NULL );

	{
		int i = 0;
		for(; i<128; i++)
		{
			if( nameOut[i] != ' ' )
				break;
		}
		if( i!= 0 )
			sprintf( nameOut, "%s", nameOut+i );
/*
		i = 0;
		for(; i<128; i++)
		{
			if( nameOut[i] == ' ' )
			{
				nameOut[i] = '\0';
				break;
			}
		}
*/
	}
	ADLASSERT( status == CL_SUCCESS );
}

void DeviceCL::getDeviceVendor( char nameOut[128] ) const
{
	cl_int status;
	status = clGetDeviceInfo( m_deviceIdx, CL_DEVICE_VENDOR, sizeof(char)*128, nameOut, NULL );
	ADLASSERT( status == CL_SUCCESS );
}

Kernel* DeviceCL::getKernel(const char* fileName, const char* funcName, const char* option, const char** srcList, int nSrc, bool cacheKernel )const
{
	return m_kernelManager->query( this, fileName, funcName, option, srcList, nSrc, cacheKernel );
}

#undef max2

};
