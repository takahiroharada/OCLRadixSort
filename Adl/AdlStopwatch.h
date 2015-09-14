/*
		2011 Takahiro Harada
*/

#if defined(WIN32)
	#include <windows.h>
	
	#define TIME_TYPE LARGE_INTEGER
	#define QUERY_FREQ(f) QueryPerformanceFrequency(&f)
	#define RECORD(t) QueryPerformanceCounter(&t)
	#define GET_TIME(t) (t).QuadPart*1000.0
	#define GET_FREQ(f)	(f).QuadPart
#else
	#include <sys/time.h> 
	
	#define TIME_TYPE timeval
	#define QUERY_FREQ(f) f.tv_sec = 1
	#define RECORD(t) gettimeofday(&t, 0)
	#define GET_TIME(t) ((t).tv_sec*1000.0+(t).tv_usec/1000.0)
	#define GET_FREQ(f)	1.0
#endif


namespace adl
{

struct StopwatchBase
{
	__inline
	StopwatchBase(): m_device(0){}
	__inline
	StopwatchBase( const Device* deviceData ){ init(deviceData); }
	__inline
	virtual ~StopwatchBase(){}

//	__inline
	virtual void init( const Device* deviceData ){}
//	__inline
	virtual void start() = 0;
//	__inline
	virtual void split() = 0;
//	__inline
	virtual void stop() = 0;
//	__inline
	virtual float getMs() = 0;
//	__inline
	virtual void getMs( float* times, int capacity ) = 0;
//	__inline
	int getNIntervals() const{ return m_idx-1;}

	enum
	{
		CAPACITY = 64,
	};

	const Device* m_device;
	int m_idx;
};

struct Stopwatch
{
	__inline
	Stopwatch( const Device* deviceData = NULL ) { m_impl=0; if(deviceData) init(deviceData);}
	__inline
	~Stopwatch();

	__inline
	void init( const Device* deviceData );
	__inline
	void start(){if(!m_impl) init(0); m_impl->start();}
	__inline
	void split(){m_impl->split();}
	__inline
	void stop(){m_impl->stop();}
	__inline
	float getMs(){ return m_impl->getMs();}
	__inline
	void getMs( float* times, int capacity ){m_impl->getMs(times, capacity);}
	__inline
	int getNIntervals() const{return m_impl->getNIntervals();}

	StopwatchBase* m_impl;
};

};
