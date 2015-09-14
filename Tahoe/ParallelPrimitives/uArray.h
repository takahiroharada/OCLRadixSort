/*
		2013 Takahiro Harada
*/
#pragma once

#include <Adl/Adl.h>
#include <Tahoe/Math/Math.h>
#include <Tahoe/Math/Array.h>

namespace Tahoe
{

template<class T>
class uArray : public Array<T>
{
public:
	TH_DECLARE_ALLOCATOR(uArray);
	
	uArray();

	uArray(int size);

	~uArray();

	T& operator[] (int idx );

	const T& operator[] (int idx) const;

	void pushBack(const T& elem);

	void clear();

	void setSize( int size );

	int getSize() const;

	T* begin();

	const T* begin() const;

	const adl::Buffer<T>* getGpuBuffer(const adl::Device* device);
	const adl::Buffer<T>* getGpuBuffer(const adl::Device* device) const;

	void setToLauncher( adl::Launcher& launcher );

	void setToLauncher( adl::Launcher& launcher ) const;

	void setDataIsClean() { m_status = STATUS_CLEAN; }

protected:
	void prepareAccessCpu();
	void prepareAccessGpu(const adl::Device* device);

	enum UStatus
	{
		STATUS_CPU_DIRTY,
		STATUS_GPU_DIRTY,
		STATUS_CLEAN, 
		STATUS_UNINITIALIZED,
	};

//	bool m_isGpuDirty;
//	bool m_isCpuDirty;
	adl::Buffer<T>* m_gpuBuff;
	UStatus m_status;
};

template<class T>
uArray<T>::uArray() : Array<T>()
{
	m_status = STATUS_CLEAN;
	m_gpuBuff = 0;
}

template<class T>
uArray<T>::uArray(int size) : Array<T>(size)
{
	m_status = STATUS_UNINITIALIZED;
	m_gpuBuff = 0;
}

template<class T>
uArray<T>::~uArray()
{
	if( m_gpuBuff )
	{
		adl::DeviceUtils::waitForCompletion( m_gpuBuff->m_device );	
		delete m_gpuBuff;
	}
}

template<class T>
T& uArray<T>::operator[] (int idx)
{
	prepareAccessCpu();
	m_status = STATUS_GPU_DIRTY;
	return Array<T>::operator[] (idx);
}

template<class T>
const T& uArray<T>::operator[] (int idx) const
{
	uArray<T>* nonconst = (uArray<T>*)this;
	nonconst->prepareAccessCpu();
	return Array<T>::operator[] (idx);
}

template<class T>
void uArray<T>::pushBack( const T& elem )
{
	prepareAccessCpu();
	m_status = STATUS_GPU_DIRTY;
	Array<T>::pushBack( elem );
}

template<class T>
void uArray<T>::clear()
{
	prepareAccessCpu();
	m_status = STATUS_GPU_DIRTY;
	Array<T>::clear();
}

template<class T>
void uArray<T>::setSize( int size )
{
	if( size == Array<T>::getSize() ) return;

	prepareAccessCpu();
	m_status = STATUS_GPU_DIRTY;
	Array<T>::setSize( size );
}

template<class T>
int uArray<T>::getSize() const
{
	return Array<T>::getSize();
}

template<class T>
T* uArray<T>::begin()
{
	prepareAccessCpu();
	m_status = STATUS_GPU_DIRTY;
	return Array<T>::begin();
}

template<class T>
const T* uArray<T>::begin() const 
{
	uArray<T>* nonconst = (uArray<T>*)this;
	nonconst->prepareAccessCpu();
	return Array<T>::begin();
}


template<class T>
void uArray<T>::prepareAccessCpu()
{
	if( m_status != STATUS_CPU_DIRTY && m_status != STATUS_UNINITIALIZED ) return;

//	ADLASSERT( m_gpuBuff != 0 );

	if( m_status != STATUS_UNINITIALIZED )
	{
		m_gpuBuff->read( Array<T>::begin(), Array<T>::getSize() );
		adl::DeviceUtils::waitForCompletion( m_gpuBuff->m_device );
	}
	m_status = STATUS_CLEAN;
}

template<class T>
void uArray<T>::prepareAccessGpu(const adl::Device* device)
{
	if( m_gpuBuff && (m_status != STATUS_GPU_DIRTY && m_status != STATUS_UNINITIALIZED) ) return;

	if( m_gpuBuff )
	{
		if( m_gpuBuff->getSize() < Array<T>::getSize() )
			m_gpuBuff->setSize( Array<T>::getSize() );
	}

	if( m_gpuBuff == 0 )
	{
		m_gpuBuff = new adl::Buffer<T>( device, Array<T>::getSize() );
	}

	if( m_status != STATUS_UNINITIALIZED )
	{
		m_gpuBuff->write( Array<T>::begin(), Array<T>::getSize() );
		adl::DeviceUtils::waitForCompletion( m_gpuBuff->m_device );
	}
	m_status = STATUS_CLEAN;
}

template<class T>
const adl::Buffer<T>* uArray<T>::getGpuBuffer(const adl::Device* device)
{
	prepareAccessGpu( device );
	m_status = STATUS_CPU_DIRTY;
	return m_gpuBuff;
}

template<class T>
const adl::Buffer<T>* uArray<T>::getGpuBuffer(const adl::Device* device) const
{
	uArray<T>* nonconst = (uArray<T>*)this;
	nonconst->prepareAccessGpu( device );
	//	todo. this should STATUS_CLEAN because it is const
	nonconst->m_status = STATUS_CPU_DIRTY;
	return m_gpuBuff;
}

template<class T>
void uArray<T>::setToLauncher( adl::Launcher& launcher )
{
	const adl::Buffer<T>* ptr = getGpuBuffer( launcher.m_deviceData );
	adl::Launcher::BufferInfo bInfo = adl::Launcher::BufferInfo( ptr );
	launcher.setBuffers( &bInfo, 1 );
}

template<class T>
void uArray<T>::setToLauncher( adl::Launcher& launcher ) const
{
	const adl::Buffer<T>* ptr = getGpuBuffer( launcher.m_deviceData );
	adl::Launcher::BufferInfo bInfo = adl::Launcher::BufferInfo( ptr );
	launcher.setBuffers( &bInfo, 1 );
}

};
