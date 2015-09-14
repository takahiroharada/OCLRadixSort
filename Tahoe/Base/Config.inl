/*
		2013 Takahiro Harada
*/
//#include <Tahoe/Tahoe.h>

namespace Tahoe
{

enum LogFilter
{
	LOG_BASE		=	1,
	LOG_ERROR		=	1<<1,
	LOG_DEBUG		=	1<<2,
	LOG_IO          =   1<<3,
	LOG_GPU			=	1<<4,
	LOG_MATERIAL	=	1<<5,
	LOG_GEOMETRY	=	1<<6,
	LOG_TEXTURE		=	1<<7,
	LOG_LIGHT		=	1<<8,
	LOG_VOLUME		=	1<<9,
            
	LOG_ALL         =   0x7fffffff,
};

class LogWriter
{
	public:
		void print(unsigned int filter, const char *fmt, ...)
		{
			if( !m_hasInitialized )
				init();

			const char* tag[] = {"Base", "Error", "Debug", "Io", "Gpu", "Mat", "Geom", "Tex", "Light", "Volume"};

			if( filter & m_filters )
			{
				FILE* file = fopen(m_logFilePath, "a");
				if( file )
				{
					fprintf(file,"%5s:	", tag[bitToIdx(filter)]);
					va_list arg;
					va_start(arg, fmt);
					vfprintf(file, fmt, arg);
					va_end(arg);
					fclose( file );
				}
			}
		}

		void resetFilter()
		{
			m_filters = 0;
		}
		void addFilter(LogFilter filter)
		{
			m_filters |= filter;
		}
		unsigned int getFilter() const
		{
			return m_filters;
		}

		static
		LogWriter& getInstance()
		{
			static LogWriter s_writer;
			return s_writer;
		}
		void setLogPath( const char* path )
		{
			sprintf(m_logFilePath, "%s/%s", path,TH_LOG_FILE);
			m_hasInitialized = 0;
		}

	private:
		LogWriter(): m_hasInitialized(0)
		{
			setLogPath( "./" );
		}

		void init()
		{
			FILE* file = fopen(m_logFilePath, "w");
			if( file )
				fclose( file );
			m_filters = 0;
#if defined(_DEBUG)
			m_filters = LOG_ALL;
#endif
			m_hasInitialized = 1;
		}

		~LogWriter()
		{
		}

		inline
		int bitToIdx(unsigned int bits)
		{
		    int idx = 0;
		    for(int i=0; i<32; i++)
		    {
		        idx += (bits&(1<<i))?i:0;
		    }
		    return idx;
		}


	protected:
		unsigned int m_hasInitialized;
		unsigned int m_filters;
		char m_logFilePath[512];

};

};
