#ifndef _LDAINFER_
#define _LDAINFER_

#include <vector>
#include <string>
#include <map>

#define  VOC_SIZE   68787L
#define  DOC_CNT    2567948L
#define  TOPIC_DIM  50 

typedef std::vector<long> Sentence;
typedef float SentenceTopicVec[TOPIC_DIM];

struct LDAModelWordInfo {
    int nw[VOC_SIZE][TOPIC_DIM];
    int nwsum [TOPIC_DIM];
};

// Total word is 68787

struct LDAWordMapping {
    char    szWord[32];  // the max len of the word is 30 in the mapping file
    int     wordId;
};


class LDAInfer {

    public:
        LDAInfer (const char* ldaModelWordInfoFile, const char* ldaDictFile,\
                  double dAlpha = 1.0, double dBeta = 0.1, int iterCnt = 100); 
        ~LDAInfer (); 
        bool inferSentence (const Sentence& sentence, float* senTopicVec);
        long ldaWordId (const char* word);

    private:
        long loadLDADict (const char* ldaDictFile);
        int  initInfer (const Sentence& sentence);
        void infer (const Sentence& sentence);
        int  inferSampling (const Sentence& sentence, int n);
        void computeTheta (float* senTopicVec);

    private:
        float alpha;
        float beta;
        float m_fProb[TOPIC_DIM];
        int   m_iIterCnt;
        int** m_piMatrixWordTopic;
        int*  m_piVecWordZ;
        int   m_iVecWordTopicSum[TOPIC_DIM];
        int   m_iVecDocTopic[TOPIC_DIM];
        int   m_iDocTopicSum;
        int   m_iSentenceVocSize;
        std::map<long,int>  m_SentenceDict;
        LDAModelWordInfo*  m_pLDAModelWordInfo;
        LDAWordMapping*  m_pLDAWordMapping;

};

#endif
