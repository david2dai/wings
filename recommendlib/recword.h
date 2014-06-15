#ifndef RECWORD_H
#define RECWORD_H

#include <string>
#include <vector>

//#include "dictionary.h"
#include "dictionary_new.h"

using std::string;
using std::vector;

// Return to caller info
struct WordInfo {
    char    word[MAX_WORD_LEN]; 
    float   sim;
    int     pos;
    float   posScore;
    long    id;
    int     freq;
};

class RecWord {

    public:
        RecWord (const char* szDictFolder);
        ~RecWord ();
      
        long searchWords (const char* origWord, // original word
                          vector<WordInfo>& retWords); // recommended words 
        void addWord2LocalContext (const char* szWord);

    private:
        int normalize (float* fVec, int dim);

    private:
        vector<string> m_localContextWordsVec;
        vector<long> m_localContextWordIdsVec;

        float m_fOrigWordVec[VOC_VEC_DIM];
        float m_fLocalContextVec[VOC_VEC_DIM];
        float m_fOrigWordAndContextVec[VOC_VEC_DIM];

        static const unsigned int REC_WORD_COUNT;
        //Dictionary& m_dict;
        DictionaryNew& m_dict;
};

#endif
