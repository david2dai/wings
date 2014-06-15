#include "chseg.h"

#include <iostream>

using std::cout;
using std::endl;

ChSeg::ChSeg ()
    :m_dict (Dictionary::GetInstance ()) {

    //constructor
}

void ChSeg::segUTF8Str (const char* szSrc, vector<string>& words) {

    string str(szSrc);

    long wordId = m_dict.getWordId (str.c_str ());
    int len = str.length ();

    if (len%3 != 0)
        return;

    //cout << len << endl;
    
    if (-1!=wordId || len<=3) {
        words.push_back (str);
        return;
    }

    for (int i=0;i<len;) {
       for (int j=len-1;j>=i+2;) {

            if (j-i+1==3) {
                words.push_back (str.substr (i,j-i+1).c_str ());
                //cout << "1" << str.substr (i,j-i+1) << endl;
                i = j+1;
                break;
            }

            if ( -1L==m_dict.getWordId (str.substr (i,j-i+1).c_str ()) ) {
                //cout << "2" << str.substr (i,j-i+1) << endl;
                j -= 3;
            } else {
                words.push_back (str.substr (i,j-i+1));
                //cout << "3" << str.substr (i,j-i+1) << endl;
                i = j+1;
                break;
            }

        }

    }

    return;
}
