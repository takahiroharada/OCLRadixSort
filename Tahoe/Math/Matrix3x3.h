/*
		2011 Takahiro Harada
*/

#ifndef MATRIX3X3_H
#define MATRIX3X3_H

#include <Tahoe/Math/Math.h>

///////////////////////////////////////
//	Matrix3x3
///////////////////////////////////////
namespace Tahoe
{

typedef 
_MEM_CLASSALIGN16 struct
{
	_MEM_ALIGNED_ALLOCATOR16;
	float4 m_row[3];
}Matrix3x3;

__inline
Matrix3x3 mtZero();

__inline
Matrix3x3 mtIdentity();

__inline
Matrix3x3 mtDiagonal(float a, float b, float c);

__inline
Matrix3x3 mtTranspose(const Matrix3x3& m);

__inline
Matrix3x3 mtMul(const Matrix3x3& a, const Matrix3x3& b);

__inline
float4 mtMul1(const Matrix3x3& a, const float4& b);

__inline
Matrix3x3 mtMul2(float a, const Matrix3x3& b);

__inline
float4 mtMul3(const float4& b, const Matrix3x3& a);

__inline
Matrix3x3 mtInvert(const Matrix3x3& m);

__inline
Matrix3x3 mtZero()
{
	Matrix3x3 m;
	m.m_row[0] = make_float4(0.f);
	m.m_row[1] = make_float4(0.f);
	m.m_row[2] = make_float4(0.f);
	return m;
}

__inline
Matrix3x3 mtIdentity()
{
	Matrix3x3 m;
	m.m_row[0] = make_float4(1,0,0);
	m.m_row[1] = make_float4(0,1,0);
	m.m_row[2] = make_float4(0,0,1);
	return m;
}

__inline
Matrix3x3 mtDiagonal(float a, float b, float c)
{
	Matrix3x3 m;
	m.m_row[0] = make_float4(a,0,0);
	m.m_row[1] = make_float4(0,b,0);
	m.m_row[2] = make_float4(0,0,c);
	return m;
}

__inline
Matrix3x3 mtTranspose(const Matrix3x3& m)
{
	Matrix3x3 out;
	out.m_row[0] = make_float4(m.m_row[0].s[0], m.m_row[1].s[0], m.m_row[2].s[0], 0.f);
	out.m_row[1] = make_float4(m.m_row[0].s[1], m.m_row[1].s[1], m.m_row[2].s[1], 0.f);
	out.m_row[2] = make_float4(m.m_row[0].s[2], m.m_row[1].s[2], m.m_row[2].s[2], 0.f);
	return out;
}

__inline
Matrix3x3 mtMul(const Matrix3x3& a, const Matrix3x3& b)
{
	Matrix3x3 transB;
	transB = mtTranspose( b );
	Matrix3x3 ans;
	for(int i=0; i<3; i++)
	{
		ans.m_row[i].s[0] = dot3F4(a.m_row[i],transB.m_row[0]);
		ans.m_row[i].s[1] = dot3F4(a.m_row[i],transB.m_row[1]);
		ans.m_row[i].s[2] = dot3F4(a.m_row[i],transB.m_row[2]);
	}
	return ans;
}

__inline
float4 mtMul1(const Matrix3x3& a, const float4& b)
{
	float4 ans;
	ans.s[0] = dot3F4( a.m_row[0], b );
	ans.s[1] = dot3F4( a.m_row[1], b );
	ans.s[2] = dot3F4( a.m_row[2], b );
	return ans;
}

__inline
Matrix3x3 mtMul2(float a, const Matrix3x3& b)
{
	Matrix3x3 ans;
	ans.m_row[0] = a*b.m_row[0];
	ans.m_row[1] = a*b.m_row[1];
	ans.m_row[2] = a*b.m_row[2];
	return ans;
}

__inline
float4 mtMul3(const float4& a, const Matrix3x3& b)
{
	float4 ans;
	ans.x = a.x*b.m_row[0].x + a.y*b.m_row[1].x + a.z*b.m_row[2].x;
	ans.y = a.x*b.m_row[0].y + a.y*b.m_row[1].y + a.z*b.m_row[2].y;
	ans.z = a.x*b.m_row[0].z + a.y*b.m_row[1].z + a.z*b.m_row[2].z;
	return ans;
}

__inline
Matrix3x3 mtInvert(const Matrix3x3& m)
{
	float det = m.m_row[0].s[0]*m.m_row[1].s[1]*m.m_row[2].s[2]+m.m_row[1].s[0]*m.m_row[2].s[1]*m.m_row[0].s[2]+m.m_row[2].s[0]*m.m_row[0].s[1]*m.m_row[1].s[2]
	-m.m_row[0].s[0]*m.m_row[2].s[1]*m.m_row[1].s[2]-m.m_row[2].s[0]*m.m_row[1].s[1]*m.m_row[0].s[2]-m.m_row[1].s[0]*m.m_row[0].s[1]*m.m_row[2].s[2];

	ADLASSERT( det != 0 );

	Matrix3x3 ans;
	ans.m_row[0].s[0] = m.m_row[1].s[1]*m.m_row[2].s[2] - m.m_row[1].s[2]*m.m_row[2].s[1];
	ans.m_row[0].s[1] = m.m_row[0].s[2]*m.m_row[2].s[1] - m.m_row[0].s[1]*m.m_row[2].s[2];
	ans.m_row[0].s[2] = m.m_row[0].s[1]*m.m_row[1].s[2] - m.m_row[0].s[2]*m.m_row[1].s[1];
	ans.m_row[0].w = 0.f;

	ans.m_row[1].s[0] = m.m_row[1].s[2]*m.m_row[2].s[0] - m.m_row[1].s[0]*m.m_row[2].s[2];
	ans.m_row[1].s[1] = m.m_row[0].s[0]*m.m_row[2].s[2] - m.m_row[0].s[2]*m.m_row[2].s[0];
	ans.m_row[1].s[2] = m.m_row[0].s[2]*m.m_row[1].s[0] - m.m_row[0].s[0]*m.m_row[1].s[2];
	ans.m_row[1].w = 0.f;

	ans.m_row[2].s[0] = m.m_row[1].s[0]*m.m_row[2].s[1] - m.m_row[1].s[1]*m.m_row[2].s[0];
	ans.m_row[2].s[1] = m.m_row[0].s[1]*m.m_row[2].s[0] - m.m_row[0].s[0]*m.m_row[2].s[1];
	ans.m_row[2].s[2] = m.m_row[0].s[0]*m.m_row[1].s[1] - m.m_row[0].s[1]*m.m_row[1].s[0];
	ans.m_row[2].w = 0.f;

	ans = mtMul2((1.0f/det), ans);
	return ans;
}

__inline
Matrix3x3 mtSet( const float4& row0, const float4& row1, const float4& row2 )
{
	Matrix3x3 m;
	m.m_row[0] = row0;
	m.m_row[1] = row1;
	m.m_row[2] = row2;
	return m;
}

__inline
Matrix3x3 operator+(const Matrix3x3& a, const Matrix3x3& b)
{
	Matrix3x3 out;
	out.m_row[0] = a.m_row[0] + b.m_row[0];
	out.m_row[1] = a.m_row[1] + b.m_row[1];
	out.m_row[2] = a.m_row[2] + b.m_row[2];
	return out;
}

__inline
Matrix3x3 operator-(const Matrix3x3& a, const Matrix3x3& b)
{
	Matrix3x3 out;
	out.m_row[0] = a.m_row[0] - b.m_row[0];
	out.m_row[1] = a.m_row[1] - b.m_row[1];
	out.m_row[2] = a.m_row[2] - b.m_row[2];
	return out;
}

__inline
Matrix3x3 mtGetRotationMatrix( const float4& eulerAngle )
{
	float s0 = sin(eulerAngle.x);
	float s1 = sin(eulerAngle.y);
	float s2 = sin(eulerAngle.z);

	float c0 = cos(eulerAngle.x);
	float c1 = cos(eulerAngle.y);
	float c2 = cos(eulerAngle.z);

	Matrix3x3 m = mtSet( 
		make_float4( c1*c2, -c0*s2+s0*s1*c2, s0*s2+c0*s1*c2 ),
		make_float4( c1*s2, c0*c2+s0*s1*s2, -s0*c2+c0*s1*s2 ),
		make_float4( -s1, s0*c1, c0*c1 ) );

	return m;
}


};

#endif

