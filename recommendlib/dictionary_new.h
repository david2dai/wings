#ifndef  DICTIONARY_NEW
#define  DICTIONARY_NEW

#include <vector>
#include <string>

#include <iconv.h>

using std::vector;
using std::string;

#define MAX_WORD_LEN        64 
#define VOC_VEC_DIM         100 
#define VOC_WORD_CNT_NEW    99656
#define VOC_CH_HASH_SIZE    6767
#define VOC_WORD_SPACE_SIZE 503100

// Chinese word hash
#define CC_ID(c1,c2) ((unsigned char)(c1)-176)*94+((unsigned char)(c2)-161)

// POS tag
const int POS_A = 1 << 0;
const int POS_B = 1 << 1; 
const int POS_C = 1 << 2;
const int POS_D = 1 << 3;
const int POS_E = 1 << 4; 
const int POS_F = 1 << 5; 
const int POS_K = 1 << 6; 
const int POS_L = 1 << 7; 
const int POS_M = 1 << 8; 
const int POS_N = 1 << 9; 
const int POS_NR= 1 << 10;
const int POS_NS= 1 << 11;
const int POS_NT= 1 << 12;
const int POS_NZ= 1 << 13;
const int POS_O = 1 << 14;
const int POS_P = 1 << 15; 
const int POS_Q = 1 << 16; 
const int POS_R = 1 << 17; 
const int POS_S = 1 << 18; 
const int POS_T = 1 << 19; 
const int POS_U = 1 << 20; 
const int POS_V = 1 << 21; 
const int POS_Y = 1 << 22; 
const int POS_Z = 1 << 23; 

// First Chinese word hash range is: max=6766,min=0,count=5014
struct DictIndex1 {
    int idx;
    int cnt;
};

struct DictWordInfoAndIndex2 {
    float vec[VOC_VEC_DIM];  // word vec
    int pos;  // POS tag
    long offset; // the offset of word string in words file
    int len; // word len
};

class DictionaryNew {
    private:
        // Dict index and data
        DictIndex1* m_pstIndex1;
        DictWordInfoAndIndex2* m_pstIndex2;
        char* m_pszWordsData;

        char m_szTmpWordGB [MAX_WORD_LEN];
        char m_szDictWordGB [MAX_WORD_LEN];
        char m_szTmpWordUTF8 [MAX_WORD_LEN];
       
        // for seg UTF8 String 
        char m_szBufferUTF8 [1024];
        char m_szBufferGB [1024];
        char m_szSentenceGB [1024];
    
        iconv_t cd_utf2gb;
        iconv_t cd_gb2utf;

    private:
        DictionaryNew (const char * szDictFolder);
        ~DictionaryNew ();
        
        bool conUTF8ToGB (char * utf8, char * gb, size_t gbsize);
        bool conGBToUTF8 (char * gb, char * utf8, size_t utf8size );
        void getAWord (char * start, int len, vector <string>& words);

    public:
        // Singleton
        static DictionaryNew& GetInstance(const char* szDictFolder = \
                "/usr/share/ibus-pinyin/resources/word2vec_dict");

        const float * getWordVec (int wordId);
        void getWord (int wordId, char * szUTF8Word, size_t size);
        int getWordPOS (int wordId);
        int getWordId (const char* orgUTF8Word);
        float getPOSScore (int orgWordPos, int recWordPos);
        bool segUTF8Str (const char * szSentence, vector <string>& words);
};

#endif
