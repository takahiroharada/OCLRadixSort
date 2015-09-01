#pragma once

#include <Adl/Adl.h>
#include <Tahoe/Math/Atomics.h>
#include <Tahoe/Math/Array.h>

namespace adl
{

template<typename T>
struct MultiBuffer
{
	public:
		MultiBuffer(): m_lock(0){}

		~MultiBuffer()
		{
			for(int i=0; i<m_buffers.getSize(); i++)
				delete m_buffers[i];
		}

		void setSize( const Device* device, int size )
		{
			Buffer<T>* b = getBuffer( device );
			b->setSize( size );
		}

		int getSize( const Device* device ) const 
		{
			Buffer<T>* b = getBuffer( device );
			return b->getSize();
		}

		bool hasInitialized( const Device* device )
		{
			Buffer<T>* b = 0;
			for(int i=0; i<m_buffers.getSize(); i++)
			{
				if(m_buffers[i]->m_device == device)
					return 1;
			}
			return 0;
		}

		Buffer<T>* getBuffer( const Device* device )
		{
			Buffer<T>* b = 0;
			for(int i=0; i<m_buffers.getSize(); i++)
			{
				if(m_buffers[i]->m_device == device)
					b = m_buffers[i];
			}
			if( b == 0 )
			{
				while( Tahoe::atomCmpxhg( &m_lock, 0, 1 ) == 0 ){}

				b = new Buffer<T>( device, 16 );
				m_buffers.pushBack( b );

				Tahoe::atomCmpxhg( &m_lock, 1, 0 );
			}
			return b;
		}

		void read( const Device* device, T* dst, int n )
		{
			Buffer<T>* b = getBuffer( device );
			b->read( dst, n );
		}

		void write( const Device* device, const T* src, int n, int offset = 0 )
		{
			Buffer<T>* b = getBuffer( device );
			b->write( src, n, offset );
		}

		T* getHostPtr( const Device* device, int size = -1 )
		{
			Buffer<T>* b = getBuffer( device );
			return b->getHostPtr( size );
		}

		void returnHostPtr( const Device* device, T* ptr )
		{
			Buffer<T>* b = getBuffer( device );
			b->returnHostPtr( ptr );
		}


	private:
		Tahoe::Array< Buffer<T>* > m_buffers;
		int m_lock;
};

template<typename T>
struct MultiData
{
	public:
		MultiData() : m_lock(0){}

		~MultiData()
		{
		}

		bool hasInitialized( const Device* device )
		{
			Buffer<T>* b = 0;
			for(int i=0; i<m_buffers.getSize(); i++)
			{
				if(m_buffers[i].m_device == device)
					return 1;
			}
			return 0;
		}
		T& get( const Device* device )
		{
			for(int i=0; i<m_buffers.getSize(); i++)
			{
				if(m_buffers[i].m_device == device)
					return m_buffers[i].m_data;
			}
			int idx;
			{
				while( Tahoe::atomCmpxhg( &m_lock, 0, 1 ) == 0 ){}
				Pair p;
				p.m_device = device;
				idx = m_buffers.getSize();
				m_buffers.pushBack( p );
				Tahoe::atomCmpxhg( &m_lock, 1, 0 );
			}
			return m_buffers[idx].m_data;
		}

	private:
		struct Pair
		{
			T m_data;
			const Device* m_device;
		};

		Tahoe::Array<Pair> m_buffers;
		int m_lock;
};

class CLUtils
{
public:
	template<typename T>
	static
	void acquireGLBuffer( Buffer<T>* buff )
	{
		if( !buff ) return;

		DeviceCL* cl = (DeviceCL*)buff->m_device;
		cl_int e;
		e = clEnqueueAcquireGLObjects( cl->m_commandQueue, 1, (cl_mem*)&buff->m_ptr, 0, 0, 0 );
		ADLASSERT( e == CL_SUCCESS );
	}

	template<typename T>
	static
	void releaseGLBuffer( Buffer<T>* buff )
	{
		if( !buff ) return;

		DeviceCL* cl = (DeviceCL*)buff->m_device;
		cl_int e;
		e = clEnqueueReleaseGLObjects( cl->m_commandQueue, 1, (cl_mem*)&buff->m_ptr, 0, 0, 0 );
		ADLASSERT( e == CL_SUCCESS );
	}

public:

};

};
