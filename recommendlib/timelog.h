#ifndef _TIMELOG_
#define _TIMELOG_

#include <sys/time.h>

class TimeLog {
   
    public:
        TimeLog ();
        ~TimeLog ();

        void start ();
        void end (const char * szMsg);

    private:
        struct timeval m_tv;
        struct timezone m_tz;
};

#endif
