/*
		2013 Takahiro Harada
*/
#pragma once

#include <Tahoe/Math/Math.h>
#include <Tahoe/Base/Config.h>
#include <new>

namespace Tahoe
{

class AllocatorBase
{
	public:
		struct Marker
		{
			u32 m_usedMem;

			Marker(){}
			Marker( u32 a ):m_usedMem(a){}
		};

		virtual 
		void *allocate(size_t size, const char* file, u32 line) = 0;
		virtual 
		void deallocate(void *p) = 0;
};

class DefaultAllocator : public AllocatorBase
{
	public:
		virtual 
		void *allocate(size_t size, const char* file, u32 line)
		{
			void* ptr = malloc( size );
			ADLASSERT( ptr != 0);
			return ptr;
		}

		virtual 
		void deallocate(void *p)
		{
			free( p );
		}

		static
		DefaultAllocator& getInstance()
		{
			static DefaultAllocator s_ma;
			return s_ma;
		}

		bool checkConsistency() {return 1;}

		void clear() {}

		void printStatistics() const {}

		u64 getCurrentUsage() const {return 0;}

		u64 getPeakUsage() const{return 0;}
};


#if (TH_MEM_DEBUG_LEVEL == 0 )
	#define TH_MEM_ALLOCATOR DefaultAllocator
#elif (TH_MEM_DEBUG_LEVEL >= 1 )
	#define TH_MEM_ALLOCATOR MemCheckAllocator
#endif



#define TH_DECLARE_ALLOCATOR(x) __inline \
	void* operator new(size_t size) \
	{ return TH_MEM_ALLOCATOR::getInstance().allocate( size, #x, __LINE__ ); } \
	__inline \
	void* operator new(size_t size, void* p) \
	{ return p; } \
	__inline \
	void operator delete(void* p) \
	{ TH_MEM_ALLOCATOR::getInstance().deallocate( p ); }\
	__inline \
	void operator delete(void* p, void*) \
	{ TH_MEM_ALLOCATOR::getInstance().deallocate( p ); }


};
/*
#if (TH_MEM_DEBUG_LEVEL >= 2)
#if (TH_LOG_LEVEL>=2)
inline void* operator new     ( size_t size ) { return Tahoe::TH_MEM_ALLOCATOR::getInstance().allocate( size, __FILE__,__LINE__ ); }
inline void* operator new[]   ( size_t size ) { return Tahoe::TH_MEM_ALLOCATOR::getInstance().allocate( size, __FILE__,__LINE__ ); }
inline void  operator delete  ( void* ptr   ) { Tahoe::TH_MEM_ALLOCATOR::getInstance().deallocate( ptr ); }
inline void  operator delete[]( void* ptr   ) { Tahoe::TH_MEM_ALLOCATOR::getInstance().deallocate( ptr ); }
#endif
#endif
*/
