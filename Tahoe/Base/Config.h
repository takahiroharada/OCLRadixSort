/*
		2013 Takahiro Harada
*/
#pragma once

#define TH_MEM_DEBUG_LEVEL 0


#if !defined(TH_LOG_LEVEL)
	#if defined(_DEBUG)
		#define TH_LOG_LEVEL 2
	#else
		#define TH_LOG_LEVEL 1
	#endif
#endif
#define TH_LOG_FILE "tahoe.log"



#define TH_LOG_BASE(...)    LogWriter::getInstance().print(Tahoe::LOG_BASE, __VA_ARGS__)
#define TH_LOG_ERROR(...)    LogWriter::getInstance().print(Tahoe::LOG_ERROR, __VA_ARGS__)
#define TH_LOG_DEBUG(...)    LogWriter::getInstance().print(Tahoe::LOG_DEBUG, __VA_ARGS__)
#define TH_LOG_IO(...)    LogWriter::getInstance().print(Tahoe::LOG_IO, __VA_ARGS__)
#define TH_LOG_GPU(...)     LogWriter::getInstance().print(Tahoe::LOG_GPU, __VA_ARGS__)
#define TH_LOG_MATERIAL(...)     LogWriter::getInstance().print(Tahoe::LOG_MATERIAL, __VA_ARGS__)
#define TH_LOG_GEOMETRY(...)     LogWriter::getInstance().print(Tahoe::LOG_GEOMETRY, __VA_ARGS__)
#define TH_LOG_TEXTURE(...)     LogWriter::getInstance().print(Tahoe::LOG_TEXTURE, __VA_ARGS__)
#define TH_LOG_LIGHT(...)     LogWriter::getInstance().print(Tahoe::LOG_LIGHT, __VA_ARGS__)
#define TH_LOG_VOLUME(...)     LogWriter::getInstance().print(Tahoe::LOG_VOLUME, __VA_ARGS__)



#include <Tahoe/Base/Config.inl>
