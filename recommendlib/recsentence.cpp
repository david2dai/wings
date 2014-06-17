#include "recsentence.h"
#include "ldainfer.h"
#include "cachefile.h"
#include "timelog.h"
//#include "log.h"
#define _ASCII
#include "CLucene.h"

#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>

#include <memory.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>

using namespace std;
using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::analysis::standard;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::search;
using namespace lucene::store;

int cmp ( const SentenceInfo& a, const SentenceInfo& b )
{
    if( a.fTotalScore > b.fTotalScore )  // Descending order 
        return 1;
    else
        return 0;
}

RecSentence::RecSentence (const char* indexSen,
                          const char* sentencesCorpusFileMain,
                          const char* sentencesCorpusFileOffset,
                          const char* ldaInfModelInfoFile,
                          const char* ldaInfWordMappingFile) {


    cd = iconv_open ("gb2312//IGNORE", "utf-8//IGNORE");

    // Load sentence offset info into memory 
    FILE * sentencesFileOffset = fopen (sentencesCorpusFileOffset, "rb");
    m_sentencesOffset = new SentenceOffset[DOC_CNT];
    memset (m_sentencesOffset, 0, sizeof (SentenceOffset) * DOC_CNT);
    fread (m_sentencesOffset, sizeof (SentenceOffset) * DOC_CNT, 1, sentencesFileOffset);
    fclose (sentencesFileOffset);

    analyzer = _CLNEW StandardAnalyzer ();

    indexReader = IndexReader::open (indexSen);
	indexSearcher = _CLNEW IndexSearcher (indexReader);

    m_ldaInfer = NULL;
    m_ldaInfer = new LDAInfer (ldaInfModelInfoFile, ldaInfWordMappingFile);

    m_cacheFile = NULL;
    //m_cacheFile = new CacheFile (sentencesCorpusFileMain, 32);
    m_cacheFile = new CacheFile (sentencesCorpusFileMain, 4);

}

RecSentence::~RecSentence ( )
{
    iconv_close(cd);

    _CLLDELETE(analyzer);
    indexSearcher->close (); 
    _CLLDELETE(indexSearcher);
    indexSearcher = NULL;
    indexReader->close();
    _CLLDELETE(indexReader);

    delete [] m_sentencesOffset;
    m_sentencesOffset = NULL;
    delete m_ldaInfer;
    m_ldaInfer = NULL;
    delete m_cacheFile;
    m_cacheFile = NULL;
}

bool RecSentence::conUTF8ToGB ( char * utf8, char * gb, size_t gbsize )
{
    char *in = utf8;
    size_t inl = strlen (utf8);
    char *out = gb;
    size_t outl = gbsize;

    int ret = iconv (cd, &in, &inl, &out, &outl);
    
    if (ret < 0) {
        
        return false;
    }

    return true;
}

float RecSentence::calKLDistance ( float* X, float* Y, int dim) {
    
    float ret = 0.0;
    // KL = sum ( p(Xi)[ log2(p(Xi)) - log2(p(Yi)) ]
    for (int i=0;i<dim;i++) {
        ret += X[i] * ( log (X[i]) - log (Y[i]) ) / log (2.0);
    }
    
    return ret;
}

bool RecSentence::addWord2LocalContext (const char* szWord) 
{
    // David add
    /* 
    clock_t start, finish;
    double dtime = 0;
    start = clock();
    */
    
    long wordId = m_ldaInfer->ldaWordId (szWord);

    if ( wordId != -1) {
        m_localContextVecWordId.push_back (wordId);
        m_localContextVecWordStr.push_back (szWord);
        while  (m_localContextVecWordId.size () > 4 ) {
            m_localContextVecWordId.erase(m_localContextVecWordId.begin());
            m_localContextVecWordStr.erase(m_localContextVecWordStr.begin());
        }

    }else {
        return false;
    }

    return true;
}

bool RecSentence::getSentencesFrom (std::vector<long>& senIds, //docIds in lucene
                                    std::vector<SentenceInRawData>& sentencesInRawData)
{
    long senId = -1;
    long offset = 0; // Sentence offset in corpus main file
    int senOrgLen = 0; // the orginal Sentence data length
    int senLdaLen = 0; // the Sentence lda t-assign data length
    int senDataLen = 0; // senOrgLen + 1 + senLdaLen
    char * pbuf = NULL;
    int senIdsSize = senIds.size (); 

    m_cacheFile->seekBegin ();

    //Log log ("/home/david/David/log.txt");
    
    for (int i=0;i<senIdsSize;++i) {
        senId = senIds[i];
        offset = m_sentencesOffset[senId].offset;
        senOrgLen = m_sentencesOffset[senId].orgLen;
        senLdaLen = m_sentencesOffset[senId].ldaLen;

        // qscore was saved in m_sentencesOffset.
        sentencesInRawData[i].qscore = m_sentencesOffset[senId].qScore / 10.0;
         
        senDataLen = senOrgLen + 1 + senLdaLen; 
        // strlen (sentence) + 1 ('\0') + tAssign count * sizeof (unsigned char)
        char * buffer = new char [senDataLen];

        m_cacheFile->read (buffer, offset, senDataLen);
        
        strcpy (sentencesInRawData[i].szSentence, buffer); 
        pbuf = buffer + senOrgLen + 1;
        //log.logInfo ("QScore [%f] Sen [%s]\n", 
        //sentencesInRawData[i].qscore, sentencesInRawData[i].szSentence);
        unsigned char * tAssignsAry = (unsigned char *) (pbuf);
        int totalWordCnt = senLdaLen / sizeof (unsigned char);
       
        /* 
        log.logInfo ("SenId [%ld] QScore New [%f]  Sen [%s] \n",
        senId, sentencesInRawData[i].qscore, sentencesInRawData[i].szSentence );
      
        */ 
        float alpha = 1.0;  // LDA alpha train paramter
        int tAssignCnt = 0; // the times of topic t was assigned by word

        // Calculate the sentences topic vector
        for (int j=0,k=0;j<TOPIC_DIM;++j) {
            tAssignCnt = 0; 
            while ( j == (int)tAssignsAry[k]  && k< totalWordCnt) {
                ++k;
                ++tAssignCnt;
            }

            // Use Gibbs Sampling to infer doc topic vector
            sentencesInRawData[i].fTopicVec[j] = (tAssignCnt + alpha) / (totalWordCnt + alpha * TOPIC_DIM);
        }

        delete [] buffer;
    }

    return true;
}


const int RecSentence::REC_SENTENCES_COUNT = 200;

bool RecSentence::searchSentencesAdvanced (const char*                   szOrigWordUTF8, 
                                           const char*                   szRecWordUTF8, 
                                           std::vector <SentenceInfo> &  sentenceInfos)
{
    char queryUTF8[512] = { 0 };
    char queryGB[512] = { 0 };
    //char szMSG[256] = {0};
    
    sprintf (queryUTF8, "%s %s", szOrigWordUTF8 , szRecWordUTF8 );
    
    if ( ! conUTF8ToGB ( queryUTF8, queryGB, sizeof (queryGB) ) )
        return false;

    //SentenceInRawData sentenceInRawData;
    // Search sentences by lucene from the index. 
    // Get a list sentences ID
    
    Query* q = QueryParser::parse(queryGB, "sentence_gb", analyzer);
    TopDocs* topDocs = indexSearcher->_search(q, NULL, REC_SENTENCES_COUNT);
    
    
    int scoreDocsLength = topDocs->scoreDocsLength;

    if (scoreDocsLength == 0) {
        return false;
    }

    vector<long> docIds;
    map<long,float> scores;
    vector<SentenceInRawData> sentencesInRawData(scoreDocsLength);

    float maxScore = 0.0;
    float rawScore = 0.0;

    for (int k=0;k<scoreDocsLength && k<REC_SENTENCES_COUNT;k++) {
        
        long docId = topDocs->scoreDocs[k].doc;
        docIds.push_back (docId);
        rawScore = topDocs->scoreDocs[k].score;
        scores[docId] = rawScore;
        
        if (rawScore > maxScore) {
            maxScore = rawScore;
        }

    }

    sort (docIds.begin(), docIds.end());// 0311
    // Get the sentences info: sentence original data and lda topic vector

    /*

    Log log ("/home/david/David/log.txt");
    log.logInfo ("searchSentences 11111");
    
    m_cacheFile->seekBegin ();
    log.logInfo ("searchSentences 22222");
    char buf[1024] = {0};
    memset (buf, 0x00, 1024);
    m_cacheFile->read (buf, 0, 512);
    */

    getSentencesFrom (docIds, sentencesInRawData);

    //Log mylog ("/home/david/David/log.txt");

    int senCnt = sentencesInRawData.size ();
    for (int k=0;k<senCnt;k++) {
        long docId = docIds[k];
        float fScore = scores[docId] / maxScore; // fScore is a value between 0~1
        
        SentenceInfo sentenceInfo;
        strcpy (sentenceInfo.szSentence, sentencesInRawData[k].szSentence);

        sentenceInfo.fLuceneScore = fScore;
        sentenceInfo.fLdaScore = 0.0;
        //sentenceInfo.fQScore = log (1.0 + sentencesInRawData[k].qscore / 10.0);
        sentenceInfo.fQScore = 1.0 / (1.0 + exp (-1.0*sentencesInRawData[k].qscore));
        sentenceInfo.fTotalScore = sentenceInfo.fQScore; //add

        sentenceInfo.id = docId ;
        sentenceInfo.freq = 0;

        //mylog.logInfo ("QScore [%f] Sen [%s]\n", sentenceInfo.fTotalScore, sentenceInfo.szSentence);

        // Not needed 
        //memcpy (sentenceInfo.fTopicVec, sentencesInRawData[k].fTopicVec, sizeof (sentenceInfo.fTopicVec));
        /*
        for (int j=0;j<SENTENCE_LDA_TOPIC_DIM;j++) {
            sentenceInfo.fTopicVec[j] = sentenceInRawData.fTopicVec[j];
        }
        */

        sentenceInfos.push_back(sentenceInfo);
    }
   
    /*    
    m_log.logInfo ("[%s][%s]:infer topic,Context size[%d]\n",
                __FILE__,
                __FUNCTION__,
                m_localContextVecWordId.size ());
    */

    if ( m_localContextVecWordId.size () > 0 ) {
      
        // Infer the topic vec of local context.   
        m_ldaInfer->inferSentence (m_localContextVecWordId, m_localContextTopicVec);
        
        string strContext;
        //float minLDAScore = 10000;
      
        //for debug 
        /*  
        for (int i = 0;i < m_localContextVecWordStr.size ();i++) {

            strContext += m_localContextVecWordStr[i];
            strContext += " ";
        }
        */

        float fMidPoint[SENTENCE_LDA_TOPIC_DIM] = {0.0};
        float * pfSenTopicVec = NULL;

        for (int i = 0;i < senCnt; i++) {

            sentenceInfos[i].fLdaScore = 0.0;
            pfSenTopicVec = sentencesInRawData[i].fTopicVec;

            for (int j= 0; j < SENTENCE_LDA_TOPIC_DIM; j++ ) {
                fMidPoint[j] = ( pfSenTopicVec[j] + m_localContextTopicVec[j]) / 2.0;
            }

            float KL_XM = calKLDistance (pfSenTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            float KL_YM = calKLDistance (m_localContextTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            //Jensen-Shannon
            float JS_Dis = 0.5 * ( KL_XM + KL_YM ); 
            
            sentenceInfos[i].fLdaScore = log (1.0+1.0/(JS_Dis+0.01));  // For dsc ordering
            sentenceInfos[i].fTotalScore = sentenceInfos[i].fLdaScore * sentenceInfos[i].fQScore;  
        }

    }
  
    // Not need to sort here
    // The sentences will be re-rank in PYPhoneticEditor.cc, 
    // There will use KL score and Feedback info to re-rank 
    //sort (sentenceInfos.begin (), sentenceInfos.end (), cmp);
    
    _CLLDELETE(topDocs);
    _CLLDELETE(q);

    return true;
}
