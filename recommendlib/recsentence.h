#ifndef _RECSENTENCE_
#define _RECSENTENCE_

#include <stdio.h>
#include <iconv.h>
#include <stdint.h>

#include <vector>
#include <string>

// David
// Do not want to include the CLuence.h here.
// Just include it in the .cpp file.

namespace lucene { 
    namespace index { class IndexReader;};
    namespace search { class IndexSearcher; };
    namespace analysis { namespace standard { class StandardAnalyzer;};};
    namespace store { class RAMDirectory;};
};

const int SENTENCE_LDA_TOPIC_DIM = 50;

// The sentence info for Engine
struct SentenceInfo {
    char    szSentence[256];
    float   fLuceneScore;
    float   fLdaScore;
    float   fQScore;
    float   fTotalScore;
    long    id;
    int     freq;
};

// Sentences corpus offset file record

struct SentenceOffset {
    uint32_t    offset;
    uint8_t     orgLen;    // max len is 150
    uint8_t     ldaLen;
    uint8_t     qScore;    // =qScore * 10
    uint8_t     padding; 
};

struct SentenceInRawData {
    char    szSentence[256]; // sentence max length is 150
    float   fTopicVec[SENTENCE_LDA_TOPIC_DIM];
    float   qscore;
    //char szPadding[256-SENTENCE_LDA_TOPIC_DIM*4- sizeof (float)];
    // 256 + 200 + 4 + 256-200 -4 = 512
};

class LDAInfer;
class CacheFile;

class RecSentence {

    public:
        RecSentence (const char* indexSen,
                     const char* sentencesCorpusFileMain,
                     const char* sentencesCorpusFileOffset,
                     const char* ldaInfModelInfoFile,
                     const char* ldaInfWordMappingFile);

        ~RecSentence ();
 
        bool searchSentencesAdvanced (const char*                   szOrigWordUTF8, 
                                      const char*                   szRecWordUTF8, 
                                      std::vector<SentenceInfo>&    sentenceInfos);

        bool addWord2LocalContext (const char* szWord);
        bool getSentencesFrom (std::vector<long>& senIds, 
                               std::vector<SentenceInRawData>& sentencesInRawData); 

    private:
        bool conUTF8ToGB (char* utf8, char* gb , size_t gbsize);
        float calKLDistance (float* X, float* Y, int dim);

    private:
        iconv_t cd;
     
        lucene::analysis::standard::StandardAnalyzer * analyzer;
        lucene::index::IndexReader * indexReader;
        lucene::search::IndexSearcher * indexSearcher;

        std::vector<long> m_localContextVecWordId;
        std::vector<std::string> m_localContextVecWordStr;
        float m_localContextTopicVec[SENTENCE_LDA_TOPIC_DIM];
        
        FILE * m_sentencesFileMain;
        SentenceOffset * m_sentencesOffset;
        LDAInfer * m_ldaInfer;
        CacheFile * m_cacheFile;
        
        static const int REC_SENTENCES_COUNT;
};

#endif
