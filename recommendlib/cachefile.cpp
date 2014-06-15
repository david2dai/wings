#include "cachefile.h"

#include <assert.h>
#include <memory.h>
//#include "log.h"

CacheFile::CacheFile (const char * szFileName, int size) {

    m_lCacheSize = size*1024L*1024L;
    
    m_file = fopen (szFileName, "rb");
    fseek (m_file, 0, SEEK_SET);
    
    m_pszCacheBuffer = NULL;
    m_pszCacheBuffer = new char[m_lCacheSize];
    if (m_pszCacheBuffer == NULL) {
        printf (" ERROR !!");
    }
    
    memset (m_pszCacheBuffer, 0x00, m_lCacheSize);

    m_lStart = 0L;
    m_lEnd  = 0L;
}

CacheFile::~CacheFile ( ) {

    fclose (m_file);
    delete [] m_pszCacheBuffer;
    m_pszCacheBuffer = NULL;

}

void CacheFile::read (char *szBuffer, long offset, int size) {

    if (offset >= m_lStart && offset < m_lEnd) {

        if (offset + size <= m_lEnd) { // all in cache
            readFromCache (szBuffer, offset-m_lStart, size);

        } else { // part in cache
            int sizeFirstRead = m_lEnd - offset;
            readFromCache (szBuffer, offset-m_lStart, sizeFirstRead);
            readFromFile (m_lEnd);
            readFromCache (szBuffer + sizeFirstRead, 0, size - sizeFirstRead);
        
        }

    } else { // need read from file
        readFromFile (offset);
        readFromCache (szBuffer, 0, size);
    }

}

void CacheFile::seekBegin () {

    m_lStart = 0L;
    m_lEnd = 0L;

    memset (m_pszCacheBuffer, 0x00, m_lCacheSize);
    fseek (m_file, 0, SEEK_SET);
}

void CacheFile::readFromCache (char *szBuffer, long offsetInCache, int size) {

    memcpy (szBuffer, m_pszCacheBuffer + offsetInCache, size);

}

void CacheFile::readFromFile (long newOffset) {

    // m_lEnd is the current offset in file
    fseek (m_file, newOffset - m_lEnd, SEEK_CUR);
    m_lStart = newOffset;
    memset (m_pszCacheBuffer, 0x00, m_lCacheSize);
    m_lEnd = fread (m_pszCacheBuffer, 1, m_lCacheSize, m_file); 
    //m_lEnd = fread (m_pszCacheBuffer, m_lCacheSize, 1, m_file); // Note this is an Error

    assert (m_lEnd >= 0 && m_lEnd <= m_lCacheSize);
    m_lEnd += newOffset;

}

// END
