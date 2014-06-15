#ifndef   _CACHEFILE_
#define   _CACHEFILE_

#include <stdio.h>

class CacheFile {
    
    public:
        CacheFile (const char * szFileName, int size = 32);
        ~CacheFile ();
        void read (char *szBuffer, long offset, int size);
        void seekBegin ();
    
    private:
        void readFromCache (char *szBuffer, long offsetInCache, int size);
        void readFromFile (long newOffset);
    private:
        long   m_lCacheSize;
        long   m_lStart;
        long   m_lEnd;
        FILE * m_file;
        char * m_pszCacheBuffer;

};

#endif
