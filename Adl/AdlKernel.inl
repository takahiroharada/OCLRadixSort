/*
		2011 Takahiro Harada
*/

#ifdef ADL_ENABLE_CL
	#include <Adl/CL/AdlKernelUtilsCL.inl>
#endif
#ifdef ADL_ENABLE_DX11
	#include <Adl/DX11/AdlKernelUtilsDX11.inl>
#endif

namespace adl
{

//==========================
//	KernelManager
//==========================
Kernel* KernelManager::query(const Device* dd, const char* fileName, const char* funcName, const char* option, const char** srcList, int nSrc, bool cacheKernel)
{
	const int charSize = 1024*2;
	KernelManager* s_kManager = this;

	char fullFineName[charSize];
	switch( dd->m_type )
	{
	case TYPE_CL:
#if defined(ADL_ENABLE_CL)
		sprintf(fullFineName,"%s.cl", fileName);
		break;
#endif
#if defined(ADL_ENABLE_DX11)
	case TYPE_DX11:
		sprintf(fullFineName,"%s.hlsl", fileName);
		break;
#endif
	default:
		ADLASSERT(0);
		break;
	};

	char mapName[charSize];
	{
		if( option )
			sprintf(mapName, "%d%s%s%s", (int)(long long)dd->getContext(), fullFineName, funcName, option);
		else
			sprintf(mapName, "%d%s%s", (int)(long long)dd->getContext(), fullFineName, funcName);
	}

	std::string str(mapName);

	if( !cacheKernel )
	{
		if( s_kManager->m_map.count( str ) )
		{
			Kernel* k = s_kManager->m_map[str];
			s_kManager->m_map.erase( str );
			delete k;
		}
	}

	KMap::iterator iter = s_kManager->m_map.find( str );

	Kernel* kernelOut;
	if( iter == s_kManager->m_map.end() )
	{
		kernelOut = new Kernel();

		switch( dd->m_type )
		{
#if defined(ADL_ENABLE_CL)
		case TYPE_CL:
			{
				KernelBuilder<TYPE_CL> builder;
				if( srcList )
					builder.setFromStringLists( dd, srcList, nSrc, option, fileName, cacheKernel );
//					builder.setFromSrc( dd, src, option );
				else
					builder.setFromFile( dd, fileName, option, true, cacheKernel );
				builder.createKernel( funcName, *kernelOut );
				kernelOut->m_funcName = funcName;
			}
			break;
#endif
#if defined(ADL_ENABLE_DX11)
		case TYPE_DX11:
			{
				KernelBuilder<TYPE_DX11> builder;
				if( src )
					builder.setFromSrc( dd, src, option );
				else
					builder.setFromFile( dd, fileName, option, true, cacheKernel );
				builder.createKernel( funcName, *kernelOut );
			}
			break;
#endif
		default:
			ADLASSERT(0);
			break;
		};
		s_kManager->m_map.insert( KMap::value_type(str,kernelOut) );
	}
	else
	{
		kernelOut = iter->second;
	}

	return kernelOut;
}

void KernelManager::reset()
{
	for(KMap::iterator iter = m_map.begin(); iter != m_map.end(); iter++)
	{
		Kernel* k = iter->second;
		switch( k->m_type )
		{
#if defined(ADL_ENABLE_CL)
		case TYPE_CL:
			KernelBuilder<TYPE_CL>::deleteKernel( *k );
			break;
#endif
#if defined(ADL_ENABLE_DX11)
		case TYPE_DX11:
			KernelBuilder<TYPE_DX11>::deleteKernel( *k );
			break;
#endif
		default:
			ADLASSERT(0);
			break;
		};
		delete k;
	}
	m_map.clear();
}

KernelManager::~KernelManager()
{
	for(KMap::iterator iter = m_map.begin(); iter != m_map.end(); iter++)
	{
		Kernel* k = iter->second;
		switch( k->m_type )
		{
#if defined(ADL_ENABLE_CL)
		case TYPE_CL:
			KernelBuilder<TYPE_CL>::deleteKernel( *k );
			break;
#endif
#if defined(ADL_ENABLE_DX11)
		case TYPE_DX11:
			KernelBuilder<TYPE_DX11>::deleteKernel( *k );
			break;
#endif
		default:
			ADLASSERT(0);
			break;
		};
		delete k;
	}
}

//==========================
//	Launcher
//==========================

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
	#define SELECT_LAUNCHER( type, func ) \
		switch( type ) \
		{ \
		case TYPE_CL: LauncherCL::func; break; \
		case TYPE_DX11: LauncherDX11::func; break; \
		default: ADLASSERT(0); break; \
		};
	#else
	#define SELECT_DEVICEDATA( type, func ) \
		switch( type ) \
		{ \
		case TYPE_DX11: ((DeviceDX11*)m_device)->func; break; \
		case TYPE_HOST: ((DeviceHost*)m_device)->func; break; \
		default: ADLASSERT(0); break; \
		}
	#define SELECT_LAUNCHER( type, func ) \
		switch( type ) \
		{ \
		case TYPE_DX11: LauncherDX11::func; break; \
		default: ADLASSERT(0); break; \
		};
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
	#define SELECT_LAUNCHER( type, func ) \
		switch( type ) \
		{ \
		case TYPE_CL: LauncherCL::func; break; \
		default: ADLASSERT(0); break; \
		};
	#else
	#define SELECT_DEVICEDATA( type, func ) \
		switch( type ) \
		{ \
		case TYPE_HOST: ((DeviceHost*)m_device)->func; break; \
		default: ADLASSERT(0); break; \
		}
	#define SELECT_LAUNCHER( type, func ) \
		switch( type ) \
		{ \
		default: ADLASSERT(0); break; \
		};
	#endif
#endif

__inline
SyncObject::SyncObject( const Device* device ) : m_device( device )
{
//	m_device->allocateSyncObj( m_ptr );
	SELECT_DEVICEDATA( m_device->m_type, allocateSyncObj( m_ptr ) );
}

__inline
SyncObject::~SyncObject()
{
//	m_device->deallocateSyncObj( m_ptr );
	SELECT_DEVICEDATA( m_device->m_type, deallocateSyncObj( m_ptr ) );
}
Launcher::Launcher(const Device *dd, const char *fileName, const char *funcName, const char *option)
{
	m_kernel = dd->getKernel( fileName, funcName, option );
	m_deviceData = dd;
	m_idx = 0;
	m_idxRw = 0;
}

Launcher::Launcher(const Device* dd, const Kernel* kernel)
{
	m_kernel = kernel;
	m_deviceData = dd;
	m_idx = 0;
	m_idxRw = 0;
}

void Launcher::setBuffers( BufferInfo* buffInfo, int n )
{
	SELECT_LAUNCHER( m_deviceData->m_type, setBuffers( this, buffInfo, n ) );
}

template<typename T>
void Launcher::setConst( Buffer<T>& constBuff, const T& consts )
{
	SELECT_LAUNCHER( m_deviceData->m_type, setConst( this, constBuff, consts ) );
}

template<typename T>
void Launcher::setConst( const T& consts )
{
	SELECT_LAUNCHER( m_deviceData->m_type, setConst( this, consts ) );
}

void Launcher::launch1D( int numThreads, int localSize, SyncObject* syncObj )
{
	SELECT_LAUNCHER( m_deviceData->m_type, launch2D( this, numThreads, 1, localSize, 1, syncObj ) );
}

void Launcher::launch2D(  int numThreadsX, int numThreadsY, int localSizeX, int localSizeY, SyncObject* syncObj )
{
	SELECT_LAUNCHER( m_deviceData->m_type, launch2D( this, numThreadsX, numThreadsY, localSizeX, localSizeY, syncObj ) );
}

void Launcher::serializeToFile( const char* filePath, const Launcher::ExecInfo& info )
{
	SELECT_LAUNCHER( m_deviceData->m_type, serializeToFile( this, filePath, info ) );
}

Launcher::ExecInfo Launcher::deserializeFromFile( const char* filePath, int buffCap, Buffer<char>** buffsOut, int* nBuffs )
{
	Launcher::ExecInfo info;
	SELECT_LAUNCHER( m_deviceData->m_type, deserializeFromFile( this, filePath, buffCap, buffsOut, nBuffs, info ) );
	return info;
}


#undef SELECT_LAUNCHER
#undef SELECT_DEVICEDATA
};
