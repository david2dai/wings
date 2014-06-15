#include "timelog.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


TimeLog::TimeLog ()
{

}

TimeLog::~TimeLog ()
{

}

void TimeLog::start ()
{
    gettimeofday (&m_tv , &m_tz);
}

void TimeLog::end (const char * szMsg)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday (&tv , &tz);

    FILE *outfile;
    //outfile = fopen("/home/david/David/time.log","at");
    outfile = fopen("/tmp/time.log","at");
    char buffer[512] = {0};
    
    sprintf (buffer, "[%s]Time Used MSecond:%ld\n",szMsg, (tv.tv_sec - m_tv.tv_sec) * 1000 +  (tv.tv_usec - m_tv.tv_usec )/1000);
    fwrite (buffer, strlen (buffer), 1, outfile);
    fclose (outfile);
}
