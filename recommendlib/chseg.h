#ifndef  CHSEG_H
#define  CHSEG_H

#include <vector>
#include <string>

#include "dictionary.h"

using std::vector;
using std::string;

class ChSeg {

    public:
        ChSeg ();
        void segUTF8Str (const char* szSrc, vector<string>& words);
    
    private:
        Dictionary& m_dict;

};

#endif
