/*
		2011 Takahiro Harada
*/

#include <map>
#include <string>
#include <fstream>

namespace adl
{

//==========================
//	Kernel
//==========================
struct Kernel
{
	DeviceType m_type;
	void* m_kernel;
	const char* m_funcName;
};

//==========================
//	KernelManager
//==========================
class KernelManager
{
	public:
		typedef std::map<std::string, Kernel*> KMap;

		__inline
		~KernelManager();

		__inline
		void reset();

		__inline
//		static
		Kernel* query(const Device* dd, const char* fileName, const char* funcName, const char* option = NULL, const char** srcList = NULL, int nSrc = 0, 
			bool cacheKernel = true);

	public:
		KMap m_map;	// Contains all the compiled kernels
};

struct SyncObject
{
	__inline
	SyncObject( const Device* device );
	__inline
	~SyncObject();

	const Device* m_device;
	void* m_ptr;
};

//==========================
//	Launcher
//==========================
class Launcher
{
	public:
		struct BufferInfo
		{
			BufferInfo(){}
			template<typename T>
			BufferInfo(Buffer<T>* buff, bool isReadOnly = false): m_buffer(buff), m_isReadOnly(isReadOnly){}
			template<typename T>
			BufferInfo(const Buffer<T>* buff, bool isReadOnly = false): m_buffer((void*)buff), m_isReadOnly(isReadOnly){}

			void* m_buffer;
			bool m_isReadOnly;
		};

		enum 
		{
			MAX_ARG_SIZE = 16,
			MAX_ARG_COUNT = 64,
		};

		struct Args
		{
			int m_isBuffer;
			size_t m_sizeInBytes;

			void* m_ptr;
			char m_data[MAX_ARG_SIZE];
		};

		struct ExecInfo
		{
			int m_nWIs[3];
			int m_wgSize[3];
			int m_nDim;

			ExecInfo(){}
			ExecInfo(int nWIsX, int nWIsY = 1, int nWIsZ = 1, int wgSizeX = 64, int wgSizeY = 1, int wgSizeZ = 1, int nDim = 1) 
			{
				m_nWIs[0] = nWIsX;
				m_nWIs[1] = nWIsY;
				m_nWIs[2] = nWIsZ;

				m_wgSize[0] = wgSizeX;
				m_wgSize[1] = wgSizeY;
				m_wgSize[2] = wgSizeZ;

				m_nDim = nDim;
			}
		};

		__inline
		Launcher(const Device* dd, const char* fileName, const char* funcName, const char* option = NULL);
		__inline
		Launcher(const Device* dd, const Kernel* kernel);
		__inline
		void setBuffers( BufferInfo* buffInfo, int n );
		template<typename T>
		__inline
		void setConst( Buffer<T>& constBuff, const T& consts );
		template<typename T>
		__inline
		void setConst( const T& consts );
		__inline
		void launch1D( int numThreads, int localSize = 64, SyncObject* syncObj = 0 );
		__inline
		void launch2D( int numThreadsX, int numThreadsY, int localSizeX = 8, int localSizeY = 8, SyncObject* syncObj = 0 );
		__inline
		void serializeToFile( const char* filePath, const ExecInfo& info );
		__inline
		ExecInfo deserializeFromFile( const char* filePath, int buffCap, Buffer<char>** buffsOut, int* nBuffs );
		
	public:
		enum
		{
			CONST_BUFFER_SIZE = 512,
		};

		const Device* m_deviceData;
		const Kernel* m_kernel;
		int m_idx;
		int m_idxRw;

		Args m_args[MAX_ARG_COUNT];
};

template<DeviceType TYPE>
class KernelBuilder
{
	public:

		__inline
		KernelBuilder(): m_ptr(0){}
		
		__inline
		void setFromFile( const Device* deviceData, const char* fileName, const char* option = NULL, bool addExtension = false,
			bool cacheKernel = true);

		__inline
		void setFromSrc( const Device* deviceData, const char* src, const char* option = NULL );

		__inline
		void setFromStringLists( const Device* deviceData, const char** src, int nSrc, const char* option = NULL, const char* fileName = NULL );

		__inline
		void createKernel( const char* funcName, Kernel& kernelOut );

		__inline
		~KernelBuilder();
		//	todo. implemement in kernel destructor?
		__inline
		static void deleteKernel( Kernel& kernel );

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
		void* m_ptr;
};

};
