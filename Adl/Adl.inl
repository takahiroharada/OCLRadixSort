/*
		2011 Takahiro Harada
*/

namespace adl
{

int DeviceUtils::getNDevices( DeviceType type )
{
	switch( type )
	{
#if defined(ADL_ENABLE_CL)
	case TYPE_CL:
		return DeviceCL::getNDevices();
#endif
#if defined(ADL_ENABLE_DX11)
	case TYPE_DX11:
		return DeviceDX11::getNDevices();
#endif
	default:
		return 1;
	};
}

struct SIMDTable
{
	SIMDTable(const char* n0, const char* n1, int n):m_n(n){ m_names[0] = n0; m_names[1] = n1; }

	enum
	{
		MAX_DEVICE_TYPE = 2,
	};

	const char *m_names[MAX_DEVICE_TYPE];
	int m_n;
};

static
SIMDTable s_tables[] = {
	SIMDTable( "Cayman", "AMD Radeon HD 6900 Series", 24 ),
	SIMDTable( "Cypress", "ATI Radeon HD 5800 Series", 20 ),
	};


int DeviceUtils::getNCUs( const Device* device )
{
	if( device->getType() == TYPE_CL )
	{
		return ((const DeviceCL*)device)->getNCUs();
	}

	char name[128];
	device->getDeviceName( name );

	//	todo. use clGetDeviceInfo, CL_DEVICE_MAX_COMPUTE_UNITS for OpenCL

	int n =sizeof(s_tables)/sizeof(SIMDTable);
	int nSIMD = -1;
	for(int i=0; i<n; i++)
	{
		for(int j=0; j<SIMDTable::MAX_DEVICE_TYPE; j++)
		{
			if( strstr( s_tables[i].m_names[j], name ) != 0 )
			{
				nSIMD = s_tables[i].m_n;
				return nSIMD;
			}
		}
	}
	return 20;
}

Device* DeviceUtils::allocate( DeviceType type, Config cfg )
{
	Device* deviceData;
	switch( type )
	{
#if defined(ADL_ENABLE_CL)
	case TYPE_CL:
		deviceData = new DeviceCL();
		break;
#endif
#if defined(ADL_ENABLE_DX11)
	case TYPE_DX11:
		deviceData = new DeviceDX11();
		break;
#endif
	case TYPE_HOST:
		deviceData = new DeviceHost();
		break;
	default:
		ADLASSERT( 0 );
		break;
	};
	deviceData->m_interopAvailable = 0;
	deviceData->initialize( cfg );
	return deviceData;
}

void DeviceUtils::deallocate( Device* deviceData )
{
	ADLASSERT( deviceData->getUsedMemory() == 0 );
	deviceData->release();
	delete deviceData;
}

void DeviceUtils::waitForCompletion( const Device* deviceData )
{
	deviceData->waitForCompletion();
}

void DeviceUtils::waitForCompletion( const SyncObject* syncObj )
{
	if( syncObj )
		syncObj->m_device->waitForCompletion( syncObj );
}

bool DeviceUtils::isComplete( const SyncObject* syncObj )
{
    bool ans = 1;
	if( syncObj )
		ans = syncObj->m_device->isComplete( syncObj );
    return ans;
}
    
void DeviceUtils::flush( const Device* device )
{
	device->flush();
}

#if defined(ADL_ENABLE_DX11)
	#if defined(ADL_ENABLE_CL)
	#define SELECT_DEVICEDATA( type, func ) \
		switch( type ) \
		{ \
		case TYPE_CL: ((DeviceCL*)m_device)->func; break; \
		case TYPE_DX11: ((DeviceDX11*)m_device)->func; break; \
		case TYPE_HOST: ((DeviceHost*)m_device)->func; break; \
		default: ADLASSERT(0); break; \
		}

	#define SELECT_DEVICEDATA1( deviceData, func ) \
		switch( deviceData->m_type ) \
		{ \
		case TYPE_CL: ((DeviceCL*)deviceData)->func; break; \
		case TYPE_DX11: ((DeviceDX11*)deviceData)->func; break; \
		case TYPE_HOST: ((DeviceHost*)deviceData)->func; break; \
		default: ADLASSERT(0); break; \
		}
	#else
	#define SELECT_DEVICEDATA( type, func ) \
		switch( type ) \
		{ \
		case TYPE_DX11: ((DeviceDX11*)m_device)->func; break; \
		case TYPE_HOST: ((DeviceHost*)m_device)->func; break; \
		default: ADLASSERT(0); break; \
		}

	#define SELECT_DEVICEDATA1( deviceData, func ) \
		switch( deviceData->m_type ) \
		{ \
		case TYPE_DX11: ((DeviceDX11*)deviceData)->func; break; \
		case TYPE_HOST: ((DeviceHost*)deviceData)->func; break; \
		default: ADLASSERT(0); break; \
		}
	#endif
#else
	#if defined(ADL_ENABLE_CL)
	#define SELECT_DEVICEDATA( type, func ) \
		switch( type ) \
		{ \
		case TYPE_CL: ((DeviceCL*)m_device)->func; break; \
		case TYPE_HOST: ((DeviceHost*)m_device)->func; break; \
		default: ADLASSERT(0); break; \
		}

	#define SELECT_DEVICEDATA1( deviceData, func ) \
		switch( deviceData->m_type ) \
		{ \
		case TYPE_CL: ((DeviceCL*)deviceData)->func; break; \
		case TYPE_HOST: ((DeviceHost*)deviceData)->func; break; \
		default: ADLASSERT(0); break; \
		}
	#else
	#define SELECT_DEVICEDATA( type, func ) \
		switch( type ) \
		{ \
		case TYPE_HOST: ((DeviceHost*)m_device)->func; break; \
		default: ADLASSERT(0); break; \
		}

	#define SELECT_DEVICEDATA1( deviceData, func ) \
		switch( deviceData->m_type ) \
		{ \
		case TYPE_HOST: ((DeviceHost*)deviceData)->func; break; \
		default: ADLASSERT(0); break; \
		}
	#endif
#endif

template<typename T>
Buffer<T>::Buffer()
{
	m_device = 0;
	m_size = 0;
	m_ptr = 0;

	m_dx11.m_uav = 0;
	m_dx11.m_srv = 0;

	m_cl.m_hostPtr = 0;

	m_allocated = false;
}

template<typename T>
Buffer<T>::Buffer(const Device* deviceData, u64 nElems, BufferType type )
{
	m_device = 0;
	m_allocated = false;
	allocate( deviceData, nElems, type );
}

template<typename T>
Buffer<T>::~Buffer()
{
	if( m_allocated )
	{
		if( m_device )
			SELECT_DEVICEDATA( m_device->m_type, deallocate( this ) );
	}

	m_device = 0;
	m_ptr = 0;
	m_size = 0;
}

template<typename T>
void Buffer<T>::setRawPtr( const Device* device, T* ptr, u64 size, BufferType type )
{
//	ADLASSERT( m_device == 0 );
	ADLASSERT( type == BUFFER );	//	todo. implement
	ADLASSERT( device->m_type != TYPE_DX11 );	//	todo. implement set srv, uav

	if( m_device )
	{
		ADLASSERT( m_device == device );
	}

	m_device = device;
	m_ptr = ptr;
	m_size = size;
}

template<typename T>
void Buffer<T>::allocate(const Device* deviceData, u64 nElems, BufferType type )
{
	ADLASSERT( m_device == 0 );
	m_device = deviceData;
	m_size = 0;
	m_ptr = 0;

	m_dx11.m_uav = 0;
	m_dx11.m_srv = 0;
	m_cl.m_hostPtr = 0;
	if( nElems )
	{
		SELECT_DEVICEDATA( m_device->m_type, allocate( this, nElems, type ) );
		m_allocated = true;
	}
}

template<typename T>
void Buffer<T>::write(const T* hostPtr, u64 nElems, u64 offsetNElems, SyncObject* syncObj)
{
	if( nElems==0 ) return;
	ADLASSERT( nElems+offsetNElems <= m_size );
	SELECT_DEVICEDATA( m_device->m_type, copy(this, (T*)hostPtr, nElems, offsetNElems, syncObj) );
}

template<typename T>
void Buffer<T>::read(T* hostPtr, u64 nElems, u64 offsetNElems, SyncObject* syncObj) const
{
	if( nElems==0 ) return;
	SELECT_DEVICEDATA( m_device->m_type, copy(hostPtr,this, nElems, offsetNElems, syncObj) );
}

template<typename T>
void Buffer<T>::write(const Buffer<T>& src, u64 nElems, SyncObject* syncObj)
{
	if( nElems==0 ) return;
	ADLASSERT( nElems <= m_size );
	SELECT_DEVICEDATA( m_device->m_type, copy(this, &src, nElems, syncObj) );
}

template<typename T>
void Buffer<T>::read(Buffer<T>& dst, u64 nElems, u64 offsetNElems, SyncObject* syncObj) const
{
	ADLASSERT( nElems <= m_size );
	ADLASSERT( offsetNElems == 0 );
	if( nElems==0 ) return;
	SELECT_DEVICEDATA( m_device->m_type, copy(&dst, this, nElems, syncObj) );
}

template<typename T>
void Buffer<T>::clear()
{
	SELECT_DEVICEDATA( m_device->m_type, clear(this) );
}

template<typename T>
void Buffer<T>::fill(void* pattern, int patternSize)
{
	SELECT_DEVICEDATA( m_device->m_type, fill(this, pattern, patternSize) );
}

template<typename T>
T* Buffer<T>::getHostPtr(u64 size) const
{
	T* ptr;
	SELECT_DEVICEDATA( m_device->m_type, getHostPtr(this, size, ptr) );
	return ptr;
}

template<typename T>
void Buffer<T>::returnHostPtr(T* ptr) const
{
	SELECT_DEVICEDATA( m_device->m_type, returnHostPtr(this, ptr) );
}

template<typename T>
void Buffer<T>::setSize( u64 size )
{
	ADLASSERT( m_device != 0 );
	if( !m_allocated )
	{
		ADLASSERT( m_ptr == 0 );
		SELECT_DEVICEDATA( m_device->m_type, allocate( this, size, BUFFER ) );
		m_allocated = true;
	}
	else
	{
		//	todo. add capacity to member
		if( getSize() < size )
		{
			ADLASSERT( m_allocated );

			{
				const Device* d = m_device;
				SELECT_DEVICEDATA( m_device->m_type, deallocate( this ) );
				DeviceUtils::waitForCompletion( d );
				allocate( d, size );
			}
		}
	}
}

/*
template<typename T>
Buffer<T>& Buffer<T>::operator = ( const Buffer<T>& buffer )
{
//	ADLASSERT( buffer.m_size <= m_size );

	SELECT_DEVICEDATA( m_device->m_type, copy(this, &buffer, min2( m_size, buffer.m_size) ) );

	return *this;
}
*/

template<DeviceType TYPE, bool COPY, typename T>
__inline
//static
typename adl::Buffer<T>* BufferUtils::map(const Device* device, const Buffer<T>* in, int copySize)
{
	Buffer<T>* native;
	ADLASSERT( device->m_type == TYPE );

	if( in->getType() == TYPE )
		native = (Buffer<T>*)in;
	else
	{
		ADLASSERT( copySize <= in->getSize() );
		copySize = (copySize==-1)? in->getSize() : copySize;

		if( TYPE == TYPE_HOST )
		{
			native = new Buffer<T>;
			native->m_device = device;
			native->m_ptr = in->getHostPtr( copySize );
			native->m_size = copySize;
		}
		else
		{
			native = new Buffer<T>( device, copySize );
			if( COPY )
			{
				if( in->getType() == TYPE_HOST )
					native->write( in->m_ptr, copySize );
	//			else if( native->getType() == TYPE_HOST )
	//			{
	//				in->read( native->m_ptr, copySize );
	//				DeviceUtils::waitForCompletion( in->m_device );
	//			}
				else
				{
					T* tmp = new T[copySize];
					in->read( tmp, copySize );
					DeviceUtils::waitForCompletion( in->m_device );
					native->write( tmp, copySize );
					DeviceUtils::waitForCompletion( native->m_device );
					delete [] tmp;
				}
			}
		}
	}
	return native;
}

template<bool COPY, typename T>
__inline
//static
void BufferUtils::unmap( Buffer<T>* native, const Buffer<T>* orig, int copySize )
{
	if( native != orig )
	{
		if( native->getType() == TYPE_HOST )
		{
//				Buffer<T>* dst = (Buffer<T>*)orig;
//				dst->write( native->m_ptr, copySize );
//				DeviceUtils::waitForCompletion( dst->m_device );
			orig->returnHostPtr( native->m_ptr );
		}
		else
		{
			if( COPY ) 
			{
				copySize = (copySize==-1)? min2(orig->getSize(), native->getSize()) : copySize;
				ADLASSERT( copySize <= orig->getSize() );
				if( orig->getType() == TYPE_HOST )
				{
					native->read( orig->m_ptr, copySize );
					DeviceUtils::waitForCompletion( native->m_device );
				}
				else
				{
					T* tmp = new T[copySize];
					native->read( tmp, copySize );
					DeviceUtils::waitForCompletion( native->m_device );
					Buffer<T>* dst = (Buffer<T>*)orig;
					dst->write( tmp, copySize );
					DeviceUtils::waitForCompletion( dst->m_device );
					delete [] tmp;
				}
			}
			DeviceUtils::waitForCompletion( native->m_device );
		}
		delete native;
	}
}

template<DeviceType TYPE, bool COPY, typename T>
__inline
//static
typename adl::Buffer<T>* BufferUtils::mapInplace(const Device* device, Buffer<T>* allocatedBuffer, const Buffer<T>* in, int copySize)
{
	Buffer<T>* native;
	ADLASSERT( device->m_type == TYPE );

	if( in->getType() == TYPE )
		native = (Buffer<T>*)in;
	else
	{
		ADLASSERT( copySize <= in->getSize() );
		copySize = (copySize==-1)? min2( in->getSize(), allocatedBuffer->getSize() ) : copySize;

		native = allocatedBuffer;
		if( COPY )
		{
			if( in->getType() == TYPE_HOST )
				native->write( in->m_ptr, copySize );
			else if( native->getType() == TYPE_HOST )
			{
				in->read( native->m_ptr, copySize );
				DeviceUtils::waitForCompletion( in->m_device );
			}
			else
			{
				T* tmp = new T[copySize];
				in->read( tmp, copySize );
				DeviceUtils::waitForCompletion( in->m_device );
				native->write( tmp, copySize );
				DeviceUtils::waitForCompletion( native->m_device );
				delete [] tmp;
			}
		}
	}
	return native;
}

template<bool COPY, typename T>
__inline
//static
void BufferUtils::unmapInplace( Buffer<T>* native, const Buffer<T>* orig, int copySize)
{
	if( native != orig )
	{
		if( COPY ) 
		{
			copySize = (copySize==-1)? min2(orig->getSize(), native->getSize()) : copySize;
			ADLASSERT( copySize <= orig->getSize() );
			if( orig->getType() == TYPE_HOST )
			{
				native->read( orig->m_ptr, copySize );
				DeviceUtils::waitForCompletion( native->m_device );
			}
			else if( native->getType() == TYPE_HOST )
			{
				Buffer<T>* dst = (Buffer<T>*)orig;
				dst->write( native->m_ptr, copySize );
				DeviceUtils::waitForCompletion( dst->m_device );
			}
			else
			{
				T* tmp = new T[copySize];
				native->read( tmp, copySize );
				DeviceUtils::waitForCompletion( native->m_device );
				Buffer<T>* dst = (Buffer<T>*)orig;
				dst->write( tmp, copySize );
				DeviceUtils::waitForCompletion( dst->m_device );
				delete [] tmp;
			}
		}
//			delete native;
	}
}

template<typename T>
T& HostBuffer<T>::operator[](int idx)
{
	return Buffer<T>::m_ptr[idx];
}

template<typename T>
const T& HostBuffer<T>::operator[](int idx) const
{
	return Buffer<T>::m_ptr[idx];
}

template<typename T>
HostBuffer<T>& HostBuffer<T>::operator = ( const Buffer<T>& device )
{
	ADLASSERT( device.m_size <= Buffer<T>::m_size );

	SELECT_DEVICEDATA1( device.m_device, copy( Buffer<T>::m_ptr, &device, device.m_size ) );

	return *this;
}

#undef SELECT_DEVICEDATA
#undef SELECT_DEVICEDATA1

};
