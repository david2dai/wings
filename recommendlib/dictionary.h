#ifndef DICTIONARY_H
#define DICTIONARY_H

//max word len is 33
#define MAX_WORD_LEN    64 
#define VOC_WORD_CNT    101188 
#define VOC_VEC_DIM     100 

// For loading the word2vec data
struct VocWordInfo {
    char    word[MAX_WORD_LEN]; //
    float   vec[VOC_VEC_DIM];   //400
    int     pos;
};

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

// Singleton class
class Dictionary {

    private:
        Dictionary (const char* file_name);
        Dictionary (const Dictionary & dict);
        Dictionary& operator= (const Dictionary & dict);
        //Dictionary ();
        ~Dictionary ();

    public:
        // Singleton
        static Dictionary& GetInstance(const char* fileName = \
                "/usr/share/ibus-pinyin/resources/words2vecInfo.bin");
      
        /* 
        static Dictionary& GetInstance(const char* fileName = \
                "/home/david/David/lucene/cluceneLib/recsentenceLib/recword_info_0111.bin");
        */

        // to get vec and word str
        const VocWordInfo& operator [] (long wordId);
        long getWordId (const char* orgWord);
        float getPOSScore (int orgWordPos, int recWordPos);

        // transform word2vec result to dictionary info file
        /*
        static int transformVocFormat (const char* srcFile,
                                       const char* posFile, 
                                       const char* destFile);
        */

        static int posMapping (const char * szPos);
    
    private:
        VocWordInfo* m_pVocWordsInfo;

};

#endif
