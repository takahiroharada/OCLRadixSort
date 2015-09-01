/*
		2011 Takahiro Harada
*/

#include <fstream>
#include <sys/stat.h>

namespace adl
{

static
//char s_cLDefaultCompileOption[] = "-I ..\\..\\ -x clc++";
char s_cLDefaultCompileOption[] = "-I ..\\..\\";

struct KernelCL : public Kernel
{
	cl_kernel& getKernel() { return (cl_kernel&)m_kernel; }
};

static const char* strip(const char* name, const char* pattern)
{
    size_t const patlen = strlen(pattern);
  	size_t patcnt = 0;
    const char * oriptr;
    const char * patloc;
    // find how many times the pattern occurs in the original string
    for (oriptr = name; (patloc = strstr(oriptr, pattern)); oriptr = patloc + patlen)
    {
		patcnt++;
    }
    return oriptr;
}

class FileStat
{
#if defined(WIN32)
public:
	FileStat( const char* filePath )
	{
		m_file = 0;
		m_file = CreateFile(filePath,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
		if (m_file ==INVALID_HANDLE_VALUE)
		{
			DWORD errorCode;
			errorCode = GetLastError();
			switch (errorCode)
			{
			case ERROR_FILE_NOT_FOUND:
				{
					debugPrintf("File not found %s\n", filePath);
					break;
				}
			case ERROR_PATH_NOT_FOUND:
				{
					debugPrintf("File path not found %s\n", filePath);
					break;
				}
			default:
				{
					printf("Failed reading file with errorCode = %d\n", errorCode);
					printf("%s\n", filePath);
				}
			}
		} 

	}
	~FileStat()
	{
		if( m_file!=INVALID_HANDLE_VALUE )
			CloseHandle( m_file );
	}

	bool found() const
	{
		return ( m_file!=INVALID_HANDLE_VALUE );
	}

	unsigned long long getTime()
	{
		if( m_file==INVALID_HANDLE_VALUE )
			return 0;

		unsigned long long t = 0;
		FILETIME exeTime;
		if (GetFileTime(m_file, NULL, NULL, &exeTime)==0)
		{

		}
		else
		{
			unsigned long long u = exeTime.dwHighDateTime;
			t = (u<<32) | exeTime.dwLowDateTime;
		}
		return t;
	}

private:
	HANDLE m_file;
#else
public:
	FileStat( const char* filePath )
	{
		strcpy( m_file, filePath );
	}

	bool found() const
	{
		struct stat binaryStat;
	    bool e = stat( m_file, &binaryStat );
		return e==0;
	}

	unsigned long long getTime()
	{
		struct stat binaryStat;
	    bool e = stat( m_file, &binaryStat );
		if( e != 0 )
			return 0;
		unsigned long long t = binaryStat.st_mtime;
		return t;
	}

private:
	char m_file[512];
#endif
};

static bool isFileUpToDate(const char* binaryFileName,const char* srcFileName)

{
	FileStat b(binaryFileName);

	if( !b.found() )
		return false;

	FileStat s(srcFileName);

	if( !s.found() )
	{
		//	todo. compare with exe time
		return true;
	}

	if( s.getTime() < b.getTime() )
		return true;

	return false;
}

template<>
class KernelBuilder<TYPE_CL>
{
	private:
		enum
		{
			MAX_PATH_LENGTH = 260,
		};
		const Device* m_deviceData;
#ifdef UNICODE
		wchar_t m_path[MAX_PATH_LENGTH];
#else
		char m_path[MAX_PATH_LENGTH];
#endif
	public:
		void* m_ptr;

	private:
		int m_error;

	public:
		KernelBuilder(): m_ptr(0){}

		void setError(cl_int e) { m_error = (e == CL_SUCCESS)?0:1; }
		int getError() const { return m_error; }

		static
		cl_program loadFromCache(const Device* device, const char* binaryFileName, const char* option)
		{
			cl_program program = 0;
			cl_int status = 0;
			FILE* file = fopen(binaryFileName, "rb");
			if( file )
			{
				fseek( file, 0L, SEEK_END );
				size_t binarySize = ftell( file );
				rewind( file );
				char* binary = new char[binarySize];
				size_t dummy = fread( binary, sizeof(char), binarySize, file );
				fclose( file );

				const DeviceCL* dd = (const DeviceCL*) device;
				program = clCreateProgramWithBinary( dd->m_context, 1, &dd->m_deviceIdx, &binarySize, (const unsigned char**)&binary, 0, &status );
				ADLASSERT( status == CL_SUCCESS );
				status = clBuildProgram( program, 1, &dd->m_deviceIdx, option, 0, 0 );
				ADLASSERT( status == CL_SUCCESS );

				delete [] binary;
				if( status != CL_SUCCESS )
					handleBuildError(dd->m_deviceIdx, program );
			}
			return program;
		}

		__inline
		void cacheBinaryToFile(const Device* device, cl_program& program, const char* binaryFileName)
		{
			cl_int status = 0;
			size_t binarySize;
			status = clGetProgramInfo( program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binarySize, 0 );
			ADLASSERT( status == CL_SUCCESS );

			char* binary = new char[binarySize];

			status = clGetProgramInfo( program, CL_PROGRAM_BINARIES, sizeof(char*), &binary, 0 );
			ADLASSERT( status == CL_SUCCESS );

			FILE* file = fopen(binaryFileName, "wb");
			if (file)
			{
				debugPrintf("Cached file created %s\n", binaryFileName);

				fwrite( binary, sizeof(char), binarySize, file );
				fclose( file );
			}

			delete [] binary;
		}

		__inline
		void createDirectory( const char* cacheDirName )
		{
#if defined(WIN32)
#ifdef UNICODE
			{
				WCHAR fName[128];
				MultiByteToWideChar(CP_ACP,0,cacheDirName,-1, fName, 128);
				CreateDirectory( fName, 0 );
			}
#else
			CreateDirectory(cacheDirName,0);
#endif
#else
            mkdir( cacheDirName, 0775 );
#endif
		}

		class File
		{
			public:
				__inline
				bool open(const char* fileNameWithExtension)
				{
					size_t      size;
					char*       str;

					// Open file stream
					std::fstream f(fileNameWithExtension, (std::fstream::in | std::fstream::binary));

					// Check if we have opened file stream
					if (f.is_open()) {
						size_t  sizeFile;
						// Find the stream size
						f.seekg(0, std::fstream::end);
						size = sizeFile = (size_t)f.tellg();
						f.seekg(0, std::fstream::beg);

						str = new char[size + 1];
						if (!str) {
							f.close();
							return  false;
						}

						// Read file
						f.read(str, sizeFile);
						f.close();
						str[size] = '\0';

						m_source  = str;

						delete[] str;

						return true;
					}

					return false;
				}
				const std::string& getSource() const {return m_source;}

			private:
				std::string m_source;
		};

// Bob Jenkins's One-at-a-Time hash (public domain license)
// From: http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx

static void hashString(const char *ss, const size_t size, char buf[9]) 
{
	const unsigned int hash = hashBin(ss, size);

	sprintf(buf, "%08x", hash);
}

static unsigned int hashBin(const char *s, const size_t size) 
{
	unsigned int hash = 0;

	for (unsigned int i = 0; i < size; ++i) {
		hash += *s++;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

static void getBinaryFileName(const Device* deviceData, const char* fileName, const char* option, char *binaryFileName) 
{
	char deviceName[256];
	deviceData->getDeviceName(deviceName);
	char driverVersion[256];
	const DeviceCL* dd = (const DeviceCL*) deviceData;
	clGetDeviceInfo(dd->m_deviceIdx, CL_DRIVER_VERSION, 256, &driverVersion, NULL);
	const char* strippedFileName = strip(fileName,"\\");
	strippedFileName = strip(strippedFileName,"/");
	char optionHash[9] = "0x0";
	if( fileName && option )
	{
		char tmp[256];
		sprintf(tmp, "%s%s", fileName, option);
		hashString(tmp, strlen(tmp), optionHash);
	}
	sprintf(binaryFileName, "%s/%s-%s.v.%d.%s.%s_%d.bin", s_cacheDirectory, strippedFileName, optionHash, deviceData->getBinaryFileVersion(), deviceName, driverVersion, (sizeof(void*)==4)?32:64 );
}

static
void handleBuildError(cl_device_id deviceIdx, cl_program program)
{
	char *build_log;
	size_t ret_val_size;
	cl_int status = clGetProgramBuildInfo(program, deviceIdx, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
	ADLASSERT( status == CL_SUCCESS );
	build_log = new char[ret_val_size+1];
	status = clGetProgramBuildInfo(program, deviceIdx, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
	ADLASSERT( status == CL_SUCCESS );

	build_log[ret_val_size] = '\0';
	Tahoe::TH_LOG_ERROR("CL Kernel Load Failure: %d\n", status);
	Tahoe::TH_LOG_ERROR("%s\n", build_log);
	printf("%s\n", build_log);

	ADLASSERT(0);
	delete build_log;
}

void setFromFile( const Device* deviceData, const char* fileName, const char* option, bool addExtension,
	bool cacheKernel)
{
	if( option == 0 )
		option = s_cLDefaultCompileOption;

	m_deviceData = deviceData;

	char fileNameWithExtension[256];

	if( addExtension )
		sprintf( fileNameWithExtension, "%s.cl", fileName );
	else
		sprintf( fileNameWithExtension, "%s", fileName );

	cl_program& program = (cl_program&)m_ptr;

	bool cacheBinary = cacheKernel;

	bool upToDate = false;
	
	char binaryFileName[512];
	{
		getBinaryFileName(deviceData, fileName, option, binaryFileName);
		upToDate = isFileUpToDate(binaryFileName,fileNameWithExtension);
		createDirectory( s_cacheDirectory );
	}
	if( cacheBinary && upToDate)
	{
		program = loadFromCache( deviceData, binaryFileName, option );
	}

	if( !m_ptr )
	{
		File kernelFile;
		bool result = kernelFile.open( fileNameWithExtension );
		if( !result )
		{
			debugPrintf("Kernel not found %s\n", fileNameWithExtension); fflush( stdout );
		}
		ADLASSERT( result );
		const char* source = kernelFile.getSource().c_str();
		setFromStringListsImpl( m_deviceData, &source, 1, option );

//		if( cacheBinary )
		{	//	write to binary
			cacheBinaryToFile( deviceData, program, binaryFileName );
		}
	}
}

void setFromStringListsImpl( const Device* deviceData, const char** src, int nSrc, const char* option )
{
	ADLASSERT( deviceData->m_type == TYPE_CL );
	m_deviceData = deviceData;
	const DeviceCL* dd = (const DeviceCL*) deviceData;

	cl_program& program = (cl_program&)m_ptr;
	cl_int status = 0;
//	size_t srcSize[] = {strlen( src )};
	size_t srcSize[128];
	ADLASSERT( nSrc < 128 );
	for(int i=0; i<nSrc; i++)
	{
		srcSize[i] = strlen( src[i] );
	}

	program = clCreateProgramWithSource( dd->m_context, nSrc, src, srcSize, &status );
	ADLASSERT( status == CL_SUCCESS );
	status = clBuildProgram( program, 1, &dd->m_deviceIdx, option, NULL, NULL );
	if( status != CL_SUCCESS )
		handleBuildError( dd->m_deviceIdx, program );
}

//	file name is used for time stamp check of cached binary
void setFromStringLists( const Device* deviceData, const char** src, int nSrc, const char* option, const char* fileName, bool cacheBinary )
{
	char fileNameWithExtension[256];

	sprintf( fileNameWithExtension, "%s.cl", fileName );

	bool upToDate = false;
	
	cl_program& program = (cl_program&)m_ptr;
	char binaryFileName[512];
	if( fileName )
	{
		getBinaryFileName(deviceData, fileName, option, binaryFileName);
		upToDate = isFileUpToDate(binaryFileName,fileNameWithExtension);
		createDirectory( s_cacheDirectory );
	}
	if( cacheBinary && upToDate )
	{
		program = loadFromCache( deviceData, binaryFileName, option );
	}

	if(!m_ptr )
	{
		setFromStringListsImpl( deviceData, src, nSrc, option );
//		if( cacheBinary )
		if( fileName )
		if( fileName[0] != 0 )
		{	//	write to binary
			cacheBinaryToFile( deviceData, program, binaryFileName );
		}
	}
}

static
cl_program loadFromSrc( const Device* deviceData, const char* src, const char* option, bool buildProgram = 1 )
{
	ADLASSERT( deviceData->m_type == TYPE_CL );
	const DeviceCL* dd = (const DeviceCL*) deviceData;

	cl_program program;
	cl_int status = 0;
	size_t srcSize[] = {strlen( src )};
	program = clCreateProgramWithSource( dd->m_context, 1, &src, srcSize, &status );
	ADLASSERT( status == CL_SUCCESS );
	if( buildProgram )
		status = clBuildProgram( program, 1, &dd->m_deviceIdx, option, NULL, NULL );
#if defined(TH_CL2)
	else
		status = clCompileProgram( program, 1, &dd->m_deviceIdx, option, 0,0,0,0,0 );
#endif
	if( status != CL_SUCCESS )
		handleBuildError( dd->m_deviceIdx, program );

	return program;
}

void setFromPrograms( const Device* device, cl_program* programs, int nPrograms, const char* option)
{
#if defined(OPENCL_10)
	printf("clLinkProgram is not available in OCL10\n");
	ADLASSERT(0);
#else
	ADLASSERT( device->m_type == TYPE_CL );
	const DeviceCL* cl = (const DeviceCL*) device;
	m_deviceData = device;
	cl_int e;
	cl_program& dst = (cl_program&)m_ptr;
	dst = clLinkProgram( cl->m_context, 1, &cl->m_deviceIdx, option, nPrograms, programs, 0, 0, &e );
	setError( e );
	if( e != CL_SUCCESS )
	{
		handleBuildError( cl->m_deviceIdx, dst );
	}
	setError( e );
#endif
}

~KernelBuilder()
{
	cl_program program = (cl_program)m_ptr;
	clReleaseProgram( program );
}

void createKernel( const char* funcName, Kernel& kernelOut )
{
	KernelCL* clKernel = (KernelCL*)&kernelOut;

	cl_program program = (cl_program)m_ptr;
	cl_int status = 0;
	clKernel->getKernel() = clCreateKernel(program, funcName, &status );
	ADLASSERT( status == CL_SUCCESS );
	setError( status );

	kernelOut.m_type = TYPE_CL;
}

static
void deleteKernel( Kernel& kernel )
{
	KernelCL* clKernel = (KernelCL*)&kernel;
	clReleaseKernel( clKernel->getKernel() );
}

};



class LauncherCL
{
	public:
		typedef Launcher::BufferInfo BufferInfo;

		__inline
		static void setBuffers( Launcher* launcher, BufferInfo* buffInfo, int n );
		template<typename T>
		__inline
		static void setConst( Launcher* launcher, Buffer<T>& constBuff, const T& consts );
		template<typename T>
		__inline
		static void setConst( Launcher* launcher, const T& consts );
		__inline
		static void launch2D( Launcher* launcher, int numThreadsX, int numThreadsY, int localSizeX, int localSizeY, SyncObject* syncObj );

		__inline
		static void serializeToFile( Launcher* launcher, const char* filePath, const Launcher::ExecInfo& info );
		__inline
		static void deserializeFromFile( Launcher* launcher, const char* filePath, int buffCap, Buffer<char>** buffsOut, int* nBuffs, Launcher::ExecInfo& info );
};

void LauncherCL::setBuffers( Launcher* launcher, BufferInfo* buffInfo, int n )
{
	KernelCL* clKernel = (KernelCL*)launcher->m_kernel;
	for(int i=0; i<n; i++)
	{
		Buffer<int>* buff = (Buffer<int>*)buffInfo[i].m_buffer;
//		ADLASSERT( buff->getSize() );
		cl_int status = clSetKernelArg( clKernel->getKernel(), launcher->m_idx++, sizeof(cl_mem), &buff->m_ptr );
		ADLASSERT( status == CL_SUCCESS );
		
		if(1)
		{
			Launcher::Args& arg = launcher->m_args[launcher->m_idx-1];
			arg.m_isBuffer = 1;
			arg.m_sizeInBytes = 0;
			arg.m_ptr = (void*)buff;//->m_ptr;
		}
		else
		{
			size_t sizeInByte;
			if( buff->m_ptr == 0 )
			{
				sizeInByte = 0;
				buff = 0;
			}
			else
			{
				cl_int e = clGetMemObjectInfo( (cl_mem)buff->m_ptr, CL_MEM_SIZE, sizeof(size_t), &sizeInByte, 0 );
				ADLASSERT( e == CL_SUCCESS );
			}

			Launcher::Args& arg = launcher->m_args[launcher->m_idx-1];
			arg.m_isBuffer = 1;
			arg.m_sizeInBytes = (int)sizeInByte;
			arg.m_ptr = (void*)buff;//->m_ptr;
		}
	}
}

template<typename T>
void LauncherCL::setConst( Launcher* launcher, Buffer<T>& constBuff, const T& consts )
{
	KernelCL* clKernel = (KernelCL*)launcher->m_kernel;
	cl_int status = clSetKernelArg( clKernel->getKernel(), launcher->m_idx++, sizeof(T), &consts );
	ADLASSERT( status == CL_SUCCESS );
}

template<typename T>
void LauncherCL::setConst( Launcher* launcher, const T& consts )
{
	KernelCL* clKernel = (KernelCL*)launcher->m_kernel;
	cl_int status = clSetKernelArg( clKernel->getKernel(), launcher->m_idx++, sizeof(T), &consts );
	ADLASSERT( status == CL_SUCCESS );

	{
		Launcher::Args& arg = launcher->m_args[launcher->m_idx-1];
		arg.m_isBuffer = 0;
		arg.m_sizeInBytes = (int)sizeof(T);
		memcpy( arg.m_data, &consts, sizeof(T) );
	}
}

void LauncherCL::launch2D( Launcher* launcher, int numThreadsX, int numThreadsY, int localSizeX, int localSizeY, SyncObject* syncObj )
{
	cl_event* e = 0;
	if( syncObj )
	{
		ADLASSERT( syncObj->m_device->getType() == TYPE_CL );
		e = (cl_event*)syncObj->m_ptr;

		if( *e )
		{
			cl_int status = clReleaseEvent( *e );
			ADLASSERT( status == CL_SUCCESS );
		}
	}
	KernelCL* clKernel = (KernelCL*)launcher->m_kernel;
	const DeviceCL* ddcl = (const DeviceCL*)launcher->m_deviceData;
    
    if( localSizeX > ddcl->m_maxLocalWGSize[0] ) localSizeX = ddcl->m_maxLocalWGSize[0];
    if( localSizeY > ddcl->m_maxLocalWGSize[1] ) localSizeY = ddcl->m_maxLocalWGSize[1];
    
	size_t gRange[3] = {1,1,1};
	size_t lRange[3] = {1,1,1};
	lRange[0] = localSizeX;
	lRange[1] = localSizeY;
	gRange[0] = max2((size_t)1, (numThreadsX/lRange[0])+(!(numThreadsX%lRange[0])?0:1));
	gRange[0] *= lRange[0];
	gRange[1] = max2((size_t)1, (numThreadsY/lRange[1])+(!(numThreadsY%lRange[1])?0:1));
	gRange[1] *= lRange[1];

	Stopwatch sw;
	sw.start();
	cl_int status = clEnqueueNDRangeKernel( ddcl->m_commandQueue, 
		clKernel->getKernel(), 2, NULL, gRange, lRange, 0,0,e );
	ADLASSERT( status == CL_SUCCESS );

#if defined(_DEBUG)
	DeviceUtils::waitForCompletion( launcher->m_deviceData );
#endif

	if( launcher->m_deviceData->m_enableProfiling )
	{
		DeviceUtils::waitForCompletion( launcher->m_deviceData );
		sw.stop();

		char filename[256];
		sprintf(filename, "ProfileCL.%s.%s.csv", ddcl->m_deviceName, ddcl->m_driverVersion);
		std::ofstream ofs( filename, std::ios::out | std::ios::app );

		ofs << "\"" << launcher->m_kernel->m_funcName << "\",\"" << sw.getMs()  << "\",\""  << gRange[0] << "\",\"" << gRange[1] << "\",\"" << gRange[1]  << "\"" ;
		ofs << std::endl;

		ofs.close();
	}
}

void LauncherCL::serializeToFile( Launcher* launcher, const char* filePath, const Launcher::ExecInfo& info )
{
	std::ofstream ofs;
	ofs.open( filePath, std::ios::out|std::ios::binary );

	ofs.write( (char*)&launcher->m_idx, sizeof(int) );

	for(int i=0; i<launcher->m_idx; i++)
	{
		Launcher::Args& arg = launcher->m_args[i];

		ofs.write( (char*)&arg.m_isBuffer, sizeof(int) );

		if( arg.m_isBuffer )
		{
			Buffer<int>* buff = (Buffer<int>*)arg.m_ptr;
			if( buff->m_ptr == 0 )
			{
				arg.m_sizeInBytes = 0;
				buff = 0;
			}
			else
			{
				cl_int e = clGetMemObjectInfo( (cl_mem)buff->m_ptr, CL_MEM_SIZE, sizeof(size_t), &arg.m_sizeInBytes, 0 );
				ADLASSERT( e == CL_SUCCESS );
			}

			ofs.write( (char*)&arg.m_sizeInBytes, sizeof(int) );
			if( arg.m_ptr && arg.m_sizeInBytes != 0 )
			{
				Buffer<char>* gpu = (Buffer<char>*)arg.m_ptr;
				char* h = gpu->getHostPtr( (int)(arg.m_sizeInBytes/sizeof(char)));
				DeviceUtils::waitForCompletion( launcher->m_deviceData );

				ofs.write( h, arg.m_sizeInBytes );

				gpu->returnHostPtr( h );
				DeviceUtils::waitForCompletion( launcher->m_deviceData );
			}
			else
			{
				ADLASSERT( arg.m_sizeInBytes == 0 );
			}
		}
		else
		{
			ofs.write( (char*)&arg.m_sizeInBytes, sizeof(int) );
			ofs.write( (char*)arg.m_data, arg.m_sizeInBytes );
		}
	}

	ofs.write( (char*)&info, sizeof(Launcher::ExecInfo) );

	ofs.close();
}

void LauncherCL::deserializeFromFile( Launcher* launcher, const char* filePath, int buffCap, Buffer<char>** buffsOut, int* nBuffs, Launcher::ExecInfo& infoOut )
{
	KernelCL* clKernel = (KernelCL*)launcher->m_kernel;

	std::ifstream ifs;
	ifs.open( filePath, std::ios::in|std::ios::binary);

	int nArgs;
	ifs.read( (char*)&nArgs, sizeof(int) );

    (*nBuffs) = 0;
	for(int i=0; i<nArgs; i++)
	{
		int isBuffer;
		int sizeInBytes;

		ifs.read( (char*)&isBuffer, sizeof(int) );
		ifs.read( (char*)&sizeInBytes, sizeof(int) );

		char* data = new char[sizeInBytes];

		ifs.read( data, sizeInBytes );

		if( isBuffer )
		{
			Buffer<char>* gpu;

            gpu = new Buffer<char>( launcher->m_deviceData, sizeInBytes );
			if( sizeInBytes == 0 )
			{

			}
			else
			{
				gpu->write( data, sizeInBytes );
				DeviceUtils::waitForCompletion( launcher->m_deviceData );
			}

			cl_int status = clSetKernelArg( clKernel->getKernel(), launcher->m_idx++, sizeof(cl_mem), (gpu->m_ptr)?&gpu->m_ptr:0 );
			ADLASSERT( status == CL_SUCCESS );
            
            if( (*nBuffs) < buffCap )
                buffsOut[(*nBuffs)++] = gpu;
		}
		else
		{
			cl_int status = clSetKernelArg( clKernel->getKernel(), launcher->m_idx++, sizeInBytes, data );
			ADLASSERT( status == CL_SUCCESS );
		}

		delete [] data;
	}
	ifs.read( (char*)&infoOut, sizeof(Launcher::ExecInfo) );

	ifs.close();
}



};
