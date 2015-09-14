/*
		2011 Takahiro Harada
*/

#ifndef CL_MATH_H
#define CL_MATH_H

#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <algorithm>
#include <limits>
#include <sstream>
#include <iomanip>

#include <Tahoe/Math/Error.h>

#define NEXTMULTIPLEOF(num, alignment) (((num)/(alignment) + (((num)%(alignment)==0)?0:1))*(alignment))

/*
#define _MEM_CLASSALIGN16 __declspec(align(16))
#define _MEM_ALIGNED_ALLOCATOR16 	void* operator new(size_t size) { return _aligned_malloc( size, 16 ); } \
	void operator delete(void *p) { _aligned_free( p ); } \
	void* operator new[](size_t size) { return _aligned_malloc( size, 16 ); } \
	void operator delete[](void *p) { _aligned_free( p ); } \
	void* operator new(size_t size, void* p) { return p; } \
	void operator delete(void *p, void* pp) {} 
*/
#define _MEM_CLASSALIGN16 
#define _MEM_ALIGNED_ALLOCATOR16 

namespace Tahoe
{

static
const float PI = (float)M_PI;

template <class T> inline std::string ToString(const T &t) 
{
	std::ostringstream ss;
	ss << t;
	return ss.str();
}

inline std::string ToString(const float t) 
{
	std::ostringstream ss;
	ss << std::setprecision(std::numeric_limits<float>::digits10 + 1) << t;
	return ss.str();
}

template<class T> inline
T nextPowerOf2(T n)
{
	n -= 1;
	for(int i=0; i<sizeof(T)*8; i++)
		n = n | (n>>i);
	return n+1;
}

template <class T> inline
T nextPowerOf(const T a, const T b) 
{
	const unsigned int r = a % b;
	if (r == 0)
		return a;
	else
		return a + b - r;
}

template <class T> inline
T roundUpToMultiple(const T x, const T m)
{
	return ((x + m - 1) / m) * m;
}

template <class T> inline
T radians(const T deg)
{
	return (PI / 180.f) * deg;
}

template <class T> inline
T degrees(const T rad) 
{
	return (180.f / PI) * rad;
}

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

_MEM_CLASSALIGN16
struct float4
{
	_MEM_ALIGNED_ALLOCATOR16;
	union
	{
		struct
		{
			float x,y,z,w;
		};
		struct
		{
			float s[4];
		};
//		__m128 m_quad;
	};
};

_MEM_CLASSALIGN16
struct int4
{
	_MEM_ALIGNED_ALLOCATOR16;
	union
	{
		struct
		{
			int x,y,z,w;
		};
		struct
		{
			int s[4];
		};
	};
};

_MEM_CLASSALIGN16
struct uint4
{
	_MEM_ALIGNED_ALLOCATOR16;
	union
	{
		struct
		{
			u32 x,y,z,w;
		};
		struct
		{
			u32 s[4];
		};
	};
};

struct packedfloat3
{
	packedfloat3() { }
	packedfloat3(const float v) {
		x = v;
		y = v;
		z = v;
	}
	
	packedfloat3(const float4 &v) {
		x = v.x;
		y = v.y;
		z = v.z;
	}

	union
	{
		struct
		{
			float x,y,z;
		};
		struct
		{
			float s[3];
		};
	};
};

struct uint2
{
	union
	{
		struct
		{
			u32 x,y;
		};
		struct
		{
			u32 s[2];
		};
	};
};

struct int2
{
	int2(){}
	int2( int a, int b ) { x=a; y=b; }
	union
	{
		struct
		{
			int x,y;
		};
		struct
		{
			int s[2];
		};
	};
};

struct float2
{
	union
	{
		struct
		{
			float x,y;
		};
		struct
		{
			float s[2];
		};
	};
};

struct u8x4
{
	u8 x;
	u8 y;
	u8 z;
	u8 w;
};

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

template<typename T>
__inline
T clamp( T value, T mi, T ma )
{
	return max2( min2( ma, value ), mi );
}

template<typename T>
__inline
T lerp( const T& a, const T& b, float f )
{
	return a*(1.f-f) + b*f;
}

#include <Tahoe/Math/Float4.inl>
#include <Tahoe/Math/Float2.inl>


template<typename T>
__inline
int2 operator*(const int2& b, T a)
{
	return make_int2(a*b.x, a*b.y);
}

template<typename T>
__inline
int2 operator*(T a, const int2& b)
{
	return make_int2(a*b.x, a*b.y);
}


__inline
float4 calcBaryCrd( const float4& v, const float4& v0, const float4& v1, const float4& v2 )
{
	float4 bCrd;
	{
		bCrd.z = length3( cross3( v-v0, v1-v0 ) );
		bCrd.x = length3( cross3( v-v1, v2-v1 ) );
		bCrd.y = length3( cross3( v-v2, v0-v2 ) );
		if( bCrd.x + bCrd.y + bCrd.z < FLT_EPSILON )
			bCrd = make_float4(1.f,0.f,0.f);
		else
			bCrd /= (bCrd.x + bCrd.y + bCrd.z);
		
	}
	return bCrd;
}

__inline
bool isInside( const float4 x, const float4 x0, const float4 x1, const float4 x2 )
{
	float4 v0 = x2 - x0;
	float4 v1 = x1 - x0;
	float4 v2 = x - x0;

	// Compute dot products
	float dot00 = dot3F4(v0, v0);
	float dot01 = dot3F4(v0, v1);
	float dot02 = dot3F4(v0, v2);
	float dot11 = dot3F4(v1, v1);
	float dot12 = dot3F4(v1, v2);

	// Compute barycentric coordinates
	float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	return (u >= 0) && (v >= 0) && (u + v < 1);
}

__inline
float log2( float x )
{
	float invLog2 = 1.f/logf(2.f);
	return logf(x)*invLog2;
}

template<typename T>
void swap2(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

__inline
void getBasis( const float4& up, float4& tangentOut, float4& bitangentOut )
{
	tangentOut = cross3((fabs(up.x)>0.0001f)? make_float4(0.f,1.f,0.f):make_float4(1.f,0.f,0.f), up);
	bitangentOut = cross3(up,tangentOut);

	tangentOut = normalize3( tangentOut );
	bitangentOut = normalize3( bitangentOut );
}


template<typename T>
T* addByteOffset(void* baseAddr, u32 offset)
{
	return (T*)(((u64)baseAddr)+offset);
}


struct Pair32
{
	Pair32(){}
	Pair32(u32 a, u32 b) : m_a(a), m_b(b){}

	u32 m_a;
	u32 m_b;
};

struct PtrPair
{
	PtrPair(){}
	PtrPair(void* a, void* b) : m_a(a), m_b(b){}
	template<typename T>
	PtrPair(T* a, T* b) : m_a((void*)a), m_b((void*)b){}

	void* m_a;
	void* m_b;
};

inline
u32 as_u32( float f )
{
	union f2i
	{
		float m_f;
		u32 m_u32;
	};
	f2i x; x.m_f = f;
	return x.m_u32;
}

inline
float as_float( u32 i )
{
	union f2i
	{
		float m_f;
		u32 m_u32;
	};
	f2i x; x.m_u32 = i;
	return x.m_f;
}

};

#include <Tahoe/Math/Matrix3x3.h>

#endif
