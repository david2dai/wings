#ifndef  LOG_H
#define  LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

class Log {

    public:
        Log (const char* log_file);
        ~Log ();
        void logInfo (const char* fmt, ...);

    private:
        char m_szFileName[1024];
        char m_szBuffer[1024];
};

#endif
