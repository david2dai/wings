#include "cachefile.h"
#include <assert.h>
#include <memory.h>
#include <iostream>

using namespace std;

int main (void) {

    CacheFile * cacheFile = new CacheFile ("/usr/share/ibus-pinyin/resources/sentencesCorpusMain.bin", 32);

    cout << "11" << endl;
    cacheFile->seekBegin ();
    cout << "222" << endl;
    char buf[1024] = {0};
    memset (buf, 0x00, 1024);
    cacheFile->read (buf, 0, 512);
    cout << buf << endl;
    delete cacheFile;
    cacheFile = NULL;
    assert ( 1 );

}
