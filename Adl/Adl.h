/*
		2011 Takahiro Harada
*/

#ifndef ADL_H
#define ADL_H

#pragma warning( disable : 4996 )
#include <Adl/AdlConfig.h>
//#include <Adl/AdlError.h>
#include <Tahoe/Math/Error.h>
#include <algorithm>
#include <limits.h>

namespace adl
{
typedef unsigned long long u64;

extern
char s_cacheDirectory[128];

#define ADL_SUCCESS 0
#define ADL_FAILURE 1

template<typename T>
__inline
T max2(const T& a, const T& b)
{
	return (a>b)? a:b;
}

template<typename T>
__inline
T min2(const T& a, const T& b)
{
	return (a<b)? a:b;
}

enum DeviceType
{
	TYPE_CL = 0,
	TYPE_DX11 = 1,
	TYPE_HOST,
};


struct Device;
struct SyncObject;

struct BufferBase
{
	enum BufferType
	{
		BUFFER,

		//	for dx
		BUFFER_CONST,
		BUFFER_STAGING,
		BUFFER_APPEND,
		BUFFER_RAW,
		BUFFER_W_COUNTER,
		BUFFER_INDEX,
		BUFFER_VERTEX,

		//	for cl
		BUFFER_ZERO_COPY,

	};
};

class DeviceUtils
{
	public:
		struct Config
		{
			enum DeviceType
			{
				DEVICE_GPU,
				DEVICE_CPU,
			};

			//	for CL
			enum DeviceVendor
			{
				VD_AMD,
				VD_INTEL,
				VD_NV,
			};

			Config() : m_type(DEVICE_GPU), m_deviceIdx(0), m_vendor(VD_AMD), m_clContextProperties(0){}

			DeviceType m_type;
			int m_deviceIdx;
			DeviceVendor m_vendor;
			void* m_clContextProperties;
		};

		__inline
		static
		int getNDevices( DeviceType type );
		__inline
		static
		int getNCUs( const Device* device );
		__inline
		static Device* allocate( DeviceType type, Config cfg = Config() );
		__inline
		static void deallocate( Device* deviceData );
		__inline
		static void waitForCompletion( const Device* deviceData );
		__inline
		static void waitForCompletion( const SyncObject* syncObj );
        __inline
        static bool isComplete( const SyncObject* syncObj );
		__inline
		static void flush( const Device* deviceData );
};

//==========================
//	DeviceData
//==========================
struct Kernel;

struct Device
{
	typedef DeviceUtils::Config Config;

	Device( DeviceType type ) : m_type( type ), m_memoryUsage(0), m_enableProfiling(false), m_binaryFileVersion(0){}
	virtual ~Device(){}
	
	virtual void* getContext() const { return 0; }
	virtual void initialize(const Config& cfg) = 0;
	virtual void release() = 0;
	virtual void waitForCompletion() const = 0;
	virtual void waitForCompletion( const SyncObject* syncObj ) const = 0;
	virtual bool isComplete( const SyncObject* syncObj ) const = 0;
	virtual void flush() const = 0;
	virtual void getDeviceName( char nameOut[128] ) const {}
	virtual void getDeviceVendor( char nameOut[128] ) const {}
	virtual Kernel* getKernel(const char* fileName, const char* funcName, const char* option = NULL, 
		const char** srcList = NULL, int nSrc = 0, bool cacheKernel = true ) const { ADLASSERT(0); return 0;}
	virtual u64 getUsedMemory() const { return m_memoryUsage; }
	void toggleProfiling( bool enable ) { m_enableProfiling = enable; }
	void setBinaryFileVersion( unsigned int ver ) { m_binaryFileVersion = ver; }
	unsigned int getBinaryFileVersion() const { return m_binaryFileVersion; }
	DeviceType getType() const { return m_type; }
	Config::DeviceType getProcType() const { return m_procType; }
	virtual u64 getMaxAllocationSize() const { return ULLONG_MAX; }

	DeviceType m_type;
	Config::DeviceType m_procType;
	u64 m_memoryUsage;
	bool m_interopAvailable;
	bool m_enableProfiling;
	unsigned int m_binaryFileVersion;
};

//==========================
//	Buffer
//==========================

template<typename T>
struct HostBuffer;
//	overload each deviceDatas
template<typename T>
struct Buffer : public BufferBase
{
	__inline
	Buffer();
	__inline
	Buffer(const Device* device, u64 nElems, BufferType type = BUFFER );
	__inline
	virtual ~Buffer();
	
	__inline
	void setRawPtr( const Device* device, T* ptr, u64 size, BufferType type = BUFFER );
	__inline
	void allocate(const Device* device, u64 nElems, BufferType type = BUFFER );
	__inline
	void write(const T* hostSrcPtr, u64 nElems, u64 dstOffsetNElems = 0, SyncObject* syncObj = 0);
	__inline
	void read(T* hostDstPtr, u64 nElems, u64 srcOffsetNElems = 0, SyncObject* syncObj = 0) const;
	__inline
	void write(const Buffer<T>& src, u64 nElems, SyncObject* syncObj = 0);
	__inline
	void read(Buffer<T>& dst, u64 nElems, u64 offsetNElems = 0, SyncObject* syncObj = 0) const;
    __inline
	void clear();
	__inline
	void fill(void* pattern, int patternSize);
	__inline
	T* getHostPtr(u64 size = -1) const;
	__inline
	void returnHostPtr(T* ptr) const;
	__inline
	void setSize( u64 size );
//	__inline
//	Buffer<T>& operator = (const Buffer<T>& buffer);
	__inline
	u64 getSize() const { return m_size; }

	DeviceType getType() const { ADLASSERT( m_device != 0 ); return m_device->m_type; }


	const Device* m_device;
	u64 m_size;
	T* m_ptr;
	//	for DX11
	union
	{
		struct 
		{
			void* m_uav;
			void* m_srv;
		} m_dx11;

		struct
		{
			char* m_hostPtr;
		}m_cl;
	};
	bool m_allocated;	//	todo. move this to a bit
};

class BufferUtils
{
public:
	//	map and unmap, buffer will be allocated if necessary
	template<DeviceType TYPE, bool COPY, typename T>
	__inline
	static
	typename adl::Buffer<T>* map(const Device* device, const Buffer<T>* in, int copySize = -1);

	template<bool COPY, typename T>
	__inline
	static
	void unmap( Buffer<T>* native, const Buffer<T>* orig, int copySize = -1 );

	//	map and unmap to preallocated buffer
	template<DeviceType TYPE, bool COPY, typename T>
	__inline
	static
	typename adl::Buffer<T>* mapInplace(const Device* device, Buffer<T>* allocatedBuffer, const Buffer<T>* in, int copySize = -1);

	template<bool COPY, typename T>
	__inline
	static
	void unmapInplace( Buffer<T>* native, const Buffer<T>* orig, int copySize = -1 );
};

//==========================
//	HostBuffer
//==========================
struct DeviceHost;

template<typename T>
struct HostBuffer : public Buffer<T>
{
	__inline
	HostBuffer():Buffer<T>(){}
	__inline
	HostBuffer(const Device* device, int nElems, BufferBase::BufferType type = BufferBase::BUFFER ) : Buffer<T>(device, nElems, type) {}
//	HostBuffer(const Device* deviceData, T* rawPtr, int nElems);


	__inline
	T& operator[](int idx);
	__inline
	const T& operator[](int idx) const;
	__inline
	T* begin() { return Buffer<T>::m_ptr; }

	__inline
	HostBuffer<T>& operator = (const Buffer<T>& device);
};

};

#include <Adl/AdlStopwatch.h>
#include <Adl/AdlKernel.h>
#if defined(ADL_ENABLE_CL)
	#include <Adl/CL/AdlCL.inl>
#endif
#if defined(ADL_ENABLE_DX11)
	#include <Adl/DX11/AdlDX11.inl>
#endif

#include <Adl/Host/AdlHost.inl>
#include <Adl/AdlKernel.inl>
#include <Adl/Adl.inl>


//#include <Adl/CL/AdlStopwatchCL.inl>
#if defined(ADL_ENABLE_DX11)
	#include <Adl/DX11/AdlStopwatchDX11.inl>
#endif
#include <Adl/Host/AdlStopwatchHost.inl>
#include <Adl/AdlStopwatch.inl>

#endif
