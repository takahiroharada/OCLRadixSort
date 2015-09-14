/*
		2011 Takahiro Harada
*/

#ifndef ARRAY_H
#define ARRAY_H

#include <string.h>
#if defined(__APPLE__)
	#include <sys/malloc.h>
#else
	#include <malloc.h>
#endif
#include <Tahoe/Math/Error.h>
#include <Tahoe/Math/Math.h>
#include <Tahoe/Base/Memory/AllocatorBase.h>
//#include <Tahoe/Serialization/Stream/StreamBase.h>

namespace Tahoe
{

template <typename T, typename ALLOCATOR = TH_MEM_ALLOCATOR>
class Array
{
	public:
		__inline
		Array();
		__inline
		Array(u64 size);
		__inline
		virtual ~Array();
		__inline
		T& operator[] (u64 idx);
		__inline
		const T& operator[] (u64 idx) const;
		__inline
		void pushBack(const T& elem);
		__inline
		T pop();
		__inline
		void popBack();
		__inline
		void clear();
		__inline
		void setSize(u64 size);
		__inline
		bool isEmpty() const;
		__inline
		u64 getSize() const;
		__inline
		T* begin();
		__inline
		const T* begin() const;
		__inline
		T* end();
		__inline
		const T* end() const;
		__inline
		u64 indexOf(const T& data) const;
		__inline
		void removeAt(u64 idx);
		__inline
		T& expandOne();

//		__inline
//		void serialize( StreamBase& stream ) const;
//		__inline
//		void deserialize( StreamBase& stream );


		__inline
		void* alignedMalloc(size_t size, u32 alignment)
		{
			void* ptr = ALLOCATOR::getInstance().allocate( size, "Array", __LINE__ );
			ADLASSERT( ptr != 0 );
			return ptr;
		}
		__inline
		void alignedFree( void* ptr )
		{
			ALLOCATOR::getInstance().deallocate( ptr );
		}


	private:
		Array(const Array& a){}

	protected:
		enum
		{
			DEFAULT_SIZE = 128,
			INCREASE_SIZE = 128,
		};

		T* m_data;
		u64 m_size;
		u64 m_capacity;
};

template <typename T, typename ALLOCATOR>
Array<T, ALLOCATOR>::Array()
{
	m_size = 0;
	m_capacity = DEFAULT_SIZE;
//	m_data = new T[ m_capacity ];
	m_data = (T*)alignedMalloc(sizeof(T)*m_capacity, 16);
	for(u64 i=0; i<m_capacity; i++) new(&m_data[i])T;
}

template <typename T, typename ALLOCATOR>
Array<T, ALLOCATOR>::Array(u64 size)
{
	m_size = size;
	m_capacity = size;
//	m_data = new T[ m_capacity ];
	m_data = (T*)alignedMalloc(sizeof(T)*m_capacity, 16);
	for(u64 i=0; i<m_capacity; i++) new(&m_data[i])T;
}

template <typename T, typename ALLOCATOR>
Array<T, ALLOCATOR>::~Array()
{
	if( m_data )
	{
//		delete [] m_data;
		alignedFree( m_data );
		m_data = NULL;
	}
}

template <typename T, typename ALLOCATOR>
T& Array<T, ALLOCATOR>::operator[](u64 idx)
{
	ADLASSERT( 0 <= idx );
	ADLASSERT(idx<m_size);
	return m_data[idx];
}

template <typename T, typename ALLOCATOR>
const T& Array<T, ALLOCATOR>::operator[](u64 idx) const
{
	ADLASSERT( 0 <= idx );
	ADLASSERT(idx<m_size);
	return m_data[idx];
}

template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::pushBack(const T& elem)
{
	if( m_size == m_capacity )
	{
		u64 s = m_size;
		setSize( 2*m_capacity );
		m_size = s;
	}
	m_data[ m_size++ ] = elem;
}

template <typename T, typename ALLOCATOR>
T Array<T, ALLOCATOR>::pop()
{
	ADLASSERT( m_size>0 );
	return m_data[--m_size];
}

template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::popBack()
{
	ADLASSERT( m_size>0 );
	m_size--;
}

template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::clear()
{
	m_size = 0;
}

template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::setSize(u64 size)
{
	if( size > m_capacity )
	{
		u64 oldCap = m_capacity;
		m_capacity = max2( size, m_capacity*2 );
		T* s = (T*)alignedMalloc(sizeof(T)*m_capacity, 16);
//		for(int i=0; i<m_capacity; i++) new(&s[i])T;
		memcpy( s, m_data, sizeof(T)*oldCap );
		alignedFree( m_data );
		m_data = s;
	}
	m_size = size;
}

template <typename T, typename ALLOCATOR>
bool Array<T, ALLOCATOR>::isEmpty() const
{
	return m_size==0;
}

template <typename T, typename ALLOCATOR>
u64 Array<T, ALLOCATOR>::getSize() const
{
	return m_size;
}

template <typename T, typename ALLOCATOR>
const T* Array<T, ALLOCATOR>::begin() const
{
	return m_data;
}

template <typename T, typename ALLOCATOR>
T* Array<T, ALLOCATOR>::begin()
{
	return m_data;
}

template <typename T, typename ALLOCATOR>
T* Array<T, ALLOCATOR>::end()
{
	return m_data+m_size;
}

template <typename T, typename ALLOCATOR>
const T* Array<T, ALLOCATOR>::end() const
{
	return m_data+m_size;
}

template <typename T, typename ALLOCATOR>
u64 Array<T, ALLOCATOR>::indexOf(const T& data) const
{
	for(u64 i=0; i<m_size; i++)
	{
		if( data == m_data[i] ) return i;
	}
	return -1;
}

template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::removeAt(u64 idx)
{
	ADLASSERT(idx<m_size);
	m_data[idx] = m_data[--m_size];
//	ADLASSERT( 0 );
}

template <typename T, typename ALLOCATOR>
T& Array<T, ALLOCATOR>::expandOne()
{
	setSize( m_size+1 );
	new( &m_data[ m_size-1 ] )T;
	return m_data[ m_size-1 ];
}
/*
template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::serialize( StreamBase& stream ) const
{
	u64 sizeInByte = m_size*sizeof(T);
	u64 capInByte = m_capacity*sizeof(T);
	stream.write( (char*)&sizeInByte, sizeof(u64) );
	stream.write( (char*)&capInByte, sizeof(u64) );
	stream.write( (char*)m_data, sizeof(T)*m_size );
}

template <typename T, typename ALLOCATOR>
void Array<T, ALLOCATOR>::deserialize( StreamBase& stream )
{
	u64 s, c;
	stream.read( (char*)&s, sizeof(u64) );
	stream.read( (char*)&c, sizeof(u64) );
	setSize( c );
	setSize( s );
	stream.read( (char*)m_data, sizeof(T)*m_size );
}
*/

template<typename T>
class GlobalArray : public Array<T, DefaultAllocator>
{
	public:

		virtual
		~GlobalArray(){}
};

};

#endif

