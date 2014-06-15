#include "log.h"
#include "string.h"

#define OPEN_LOG

Log::Log (const char* log_file) {

    memset (m_szFileName, 0, sizeof (m_szFileName));
    strcpy (m_szFileName, log_file);

}

Log::~Log () {


}
        
void Log::logInfo (const char* fmt, ...) {
 
#ifdef OPEN_LOG
    FILE * logfile; 
    logfile = fopen (m_szFileName, "at");
   
    va_list pArgList;
    va_start(pArgList,fmt);
    memset (m_szBuffer, 0, sizeof (m_szBuffer));

    vsprintf (m_szBuffer, fmt, pArgList);
    //sprintf (m_szBuffer, fmt, pArgList);
    //printf (fmt, pArgList);
    //vprintf
    fwrite (m_szBuffer, strlen (m_szBuffer), 1, logfile);
    fflush (logfile);
    va_end(pArgList);
    
    fclose (logfile);
#endif

    return;
}
