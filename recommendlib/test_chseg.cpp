#include "chseg.h"

#include <iostream>
#include <vector>
#include <string>

#include <stdarg.h>
#include "log.h"

/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
*/

using namespace std;

void testSeg (const char* str) {

    ChSeg chSeg;
    vector<string> words; 
    chSeg.segUTF8Str (str, words);

    cout << str << endl;
    for (int i=0;i<words.size ();++i) {
        cout << "\"" << words[i] << "\"" << " ";
    }

    cout << endl;
}

/*
void logInfo (const char* fmt, ...) {
    
    FILE * m_file = fopen ("./log2", "at");
    va_list pArgList;
    va_start(pArgList,fmt);
    char m_szBuff [1024] = {0};
    memset (m_szBuff, 0, sizeof (m_szBuff));

    cout << fmt << endl;
    sprintf (m_szBuff, fmt, pArgList);
    vprintf (fmt, pArgList);
    fwrite (m_szBuff, strlen (m_szBuff), 1, m_file);
    cout << m_szBuff << endl;
    fflush (m_file);
    va_end(pArgList);
   
    fclose (m_file); 
    return;
}

*/
int main (void) {

    testSeg ("哈尔滨工业大学计算机专业硕士");
    testSeg ("狗屎运");
    testSeg ("土豪金");
    testSeg ("金土豪");
    testSeg ("王家");
    testSeg ("家");

    //logInfo ("2[ %s %d ]\n", "hello", 12);
    Log log ("./log");
    log.logInfo ("[%s:%s:%s:%d]\n", __FUNCTION__, __FILE__,"hello", 12);

}
