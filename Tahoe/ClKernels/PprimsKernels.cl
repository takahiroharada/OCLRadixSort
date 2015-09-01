/*
		2013 Takahiro Harada
*/
#line 2 "PrimsKernels.cl"
typedef unsigned int u32;


__kernel 
void CopyIntKernel( __global int* gDst, __global int* gSrc, int n ) 
{ 
	int gIdx = get_global_id(0); 
	if( gIdx >= n ) return; 
	gDst[gIdx] = gSrc[gIdx]; 
}

__kernel 
void CopyF4Kernel( __global float4* gDst, __global float4* gSrc, int n ) 
{ 
	int gIdx = get_global_id(0); 
	if( gIdx >= n ) return; 
	gDst[gIdx] = gSrc[gIdx]; 
}



__kernel 
void FillIntKernel( __global int* gDst, int src, int n ) 
{ 
	int gIdx = get_global_id(0); 
	if( gIdx >= n ) return; 
	gDst[gIdx] = src; 
}

__kernel 
void FillU32Kernel( __global u32* gDst, u32 src, int n ) 
{ 
	int gIdx = get_global_id(0); 
	if( gIdx >= n ) return; 
	gDst[gIdx] = src; 
}

__kernel 
void FillF4Kernel( __global float4* gDst, float4 src, int n ) 
{ 
	int gIdx = get_global_id(0); 
	if( gIdx >= n ) return; 
	gDst[gIdx] = src;
}
