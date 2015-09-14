/*
		2011 Takahiro Harada
*/

#pragma once

#include <stdio.h>
#include <stdarg.h>

#if defined(ADL_DUMP_DX11_ERROR)
	#include <windows.h>
#endif
#ifdef _DEBUG
	#include <assert.h>
#endif

#if defined(TH_UNIT_TEST)
	#include <gtest/gtest.h>
#endif

#include <Tahoe/Base/Config.h>


#ifdef _DEBUG
	#ifdef _WIN32
		#define ADLASSERT(x) if(!(x)){__debugbreak(); }
	#else
		#define ADLASSERT(x) if(!(x)){assert(0); }
	#endif
	#define ADLWARN(x) { printf(x);}
#else
    #if defined(TH_UNIT_TEST)
        #define ADLASSERT(x) EXPECT_TRUE(x)
        #define ADLWARN(x) {x;}
    #else
		#define ADLASSERT(x) if(x){}
		#define ADLWARN(x) {x;}
    #endif
#endif

#define ADLCOMPILEASSERT(x) static_assert( x, "CompileTimeAssert" )
#define ADLCOMPILEASSERT1(x, msg) static_assert( x, msg )

__inline
void thDebugPrintf(const char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	vprintf(fmt, arg);
	va_end(arg);
}
#if defined(_DEBUG)
#define debugPrintfImpl(fmt,...) thDebugPrintf(fmt,__VA_ARGS__);
#define debugPrintf(...) debugPrintfImpl(__VA_ARGS__, "");Tahoe::TH_LOG_DEBUG(__VA_ARGS__);
//#define debugPrintf(fmt,...) thDebugPrintf(fmt,__VA_ARGS__); 
#else
#define debugPrintf(...) Tahoe::TH_LOG_DEBUG(__VA_ARGS__);
#endif


