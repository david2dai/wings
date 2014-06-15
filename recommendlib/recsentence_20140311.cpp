#include "recsentence.h"
#include "timelog.h"

#define _ASCII
#include "CLucene.h"

#include <iostream>
#include <string>
#include <fstream>
#include <memory.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <algorithm>
#include <map>


#include "log.h"

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
    //if( a.fLuceneScore > b.fLuceneScore)  // Descending order 
    //if( a.fTotalScore < b.fTotalScore )  // Ascending order 
    //if( a.fLdaScore < b.fLdaScore)  // Ascending order 
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

    // New method 
    m_sentencesFileMain = fopen (sentencesCorpusFileMain, "rb");
   
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

}


RecSentence::~RecSentence ( )
{
    iconv_close(cd);

    //fclose (m_sentencesFile);
    fclose (m_sentencesFileMain);
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

    /*
    FILE *outfile; 
    outfile = fopen("/home/david/David/tablog","at");
    char buffer[1024] = {0};

    finish = clock();
    dtime = (double)(finish - start) / CLOCKS_PER_SEC;

    sprintf(buffer,"[%s]:add context word[%s] sucess, size if [%d] time:[%lf]\n",\
            __FILE__, szWord, m_localContextVecWordId.size (), dtime);

    fwrite (buffer , strlen(buffer), 1 , outfile);
    fclose ( outfile );
    */

    return true;
}

bool RecSentence::getSentencesFrom (std::vector<long>& senIds, //docIds in lucene
                                    vector<SentenceInRawData>& sentencesInRawData)
{
    //long lastOffset = 0;
    //fseek (m_sentencesFileMain, 0, SEEK_SET);
    long senId = -1;
    long offset = 0; // Sentence offset in corpus main file
    int senOrgLen = 0; // the orginal Sentence data length
    int senLdaLen = 0; // the Sentence lda t-assign data length
    int senDataLen = 0; // senOrgLen + 1 + senLdaLen

    for (int i=0;i<senIds.size ();++i) {
        senId = senIds[i];
        offset = m_sentencesOffset[senId].offset;
        senOrgLen = m_sentencesOffset[senId].orgLen;
        senLdaLen = m_sentencesOffset[senId].ldaLen;

        senDataLen = senOrgLen + 1 + senLdaLen; // strlen (sentence) + 1 ('\0') + tAssign count * sizeof (int)
        char * buffer = new char [senDataLen];
        fseek (m_sentencesFileMain, offset, SEEK_SET);
        fread (buffer, senDataLen, 1, m_sentencesFileMain);
        strcpy (sentencesInRawData[i].szSentence, buffer); 
        
        int * tAssignsAry = (int *) (buffer + senOrgLen + 1);
        int totalWordCnt = senLdaLen / sizeof (int);
        
        float alpha = 1.0;  // LDA alpha train paramter
        int tAssignCnt = 0; // the times of topic t was assigned by word
        //float topicVec[TOPIC_DIM];

        // Calculate the sentences topic vector
        for (int j=0,k=0;j<TOPIC_DIM;++j) {
            tAssignCnt = 0; 
            while ( j == tAssignsAry[k]  && k< totalWordCnt) {
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
    //Log m_log ("/home/david/David/tablog");
    
    char queryUTF8[512] = { 0 };
    char queryGB[512] = { 0 };
   
    sprintf (queryUTF8, "%s %s", szOrigWordUTF8 , szRecWordUTF8 );
    
    if ( ! conUTF8ToGB ( queryUTF8, queryGB, sizeof (queryGB) ) )
        return false;

    SentenceInRawData sentenceInRawData;
    
    // Search sentences by lucene from the index. 
    // Get a list sentences ID
    Query* q = QueryParser::parse(queryGB, "sentence_gb", analyzer);
    TopDocs* topDocs = indexSearcher->_search(q, NULL, REC_SENTENCES_COUNT);
    
    size_t scoreDocsLength = topDocs->scoreDocsLength;

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

    // Get the sentences info: sentence original data and lda topic vector
    getSentencesFrom (docIds, sentencesInRawData);

    for (int k=0;k<sentencesInRawData.size ();k++) {
        long docId = docIds[k];
        float fScore = scores[docId] / maxScore; // fScore is a value between 0~1
        
        SentenceInfo sentenceInfo;
        strcpy (sentenceInfo.szSentence, sentencesInRawData[k].szSentence);
        sentenceInfo.fLuceneScore = fScore;
        sentenceInfo.fLdaScore = 0.0;
        sentenceInfo.fTotalScore = fScore;
        sentenceInfo.id = docId ;
        sentenceInfo.freq = 0;
   
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
        float minLDAScore = 10000;
        
        for (int i = 0;i < m_localContextVecWordStr.size ();i++) {

            strContext += m_localContextVecWordStr[i];
            strContext += " ";
        }

        float fMidPoint[SENTENCE_LDA_TOPIC_DIM] = {0.0};
        float * pfSenTopicVec = NULL;

        for (int i = 0;i < sentenceInfos.size (); i++) {

            sentenceInfos[i].fLdaScore = 0.0;
            pfSenTopicVec = sentencesInRawData[i].fTopicVec;

            for (int j= 0; j < SENTENCE_LDA_TOPIC_DIM; j++ ) {
                fMidPoint[j] = ( pfSenTopicVec[j] + m_localContextTopicVec[j]) / 2.0;
            }

            float KL_XM = calKLDistance (pfSenTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            float KL_YM = calKLDistance (m_localContextTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            float KL_XY = calKLDistance (m_localContextTopicVec, pfSenTopicVec, SENTENCE_LDA_TOPIC_DIM);
            //Jensen-Shannon
            float JS_Dis = 0.5 * ( KL_XM + KL_YM ); 
            
            //sentenceInfos[i].fLdaScore = -1 * JS_Dis;  // For dsc ordering
            sentenceInfos[i].fLdaScore = 1.0 / (1.0 + JS_Dis);  // For dsc ordering
            //sentenceInfos[i].fTotalScore = -1 * JS_Dis; 
            sentenceInfos[i].fTotalScore = 1.0 / (1.0 + JS_Dis);  
        }

        /*
        m_log.logInfo ("[%s][%s]:the min LDA score is [%f]\n",
                __FILE__,
                __FUNCTION__,
                minLDAScore);

        */

        // Marked 2014.02.15
        /*
        for (int i = 0;i < sentenceInfos.size (); i++) {

            sentenceInfos[i].fTotalScore = 1.0/(1.0 + sentenceInfos[i].fLdaScore-minLDAScore);
            //sentenceInfos[i].fTotalScore = sentenceInfos[i].fLuceneScore + 2.0 * sentenceInfos[i].fTotalScore;
        }
        */

    }
  
    // Not need to sort here
    // The sentences will be re-rank in PYPhoneticEditor.cc, 
    // There will use KL score and Feedback info to re-rank 
    //sort (sentenceInfos.begin (), sentenceInfos.end (), cmp);
    
    /*
    for (int i= 0;i<10 && i < sentenceInfos.size (); i++) {

        m_log.logInfo ("[%d]:[%f]:[%s]\n",
                i+1,
                sentenceInfos[i].fTotalScore,
                sentenceInfos[i].szSentence);

    }
    */

    /*
    for (int i= 0;i < sentenceInfos.size (); i++) {

        m_log.logInfo ("[%s][%s]:SenId[%d],luceneS[%f],ldaS[%f],total[%f],recSen[%s]\n",
                __FILE__,
                __FUNCTION__,
                docIds[i],
                sentenceInfos[i].fLuceneScore,
                sentenceInfos[i].fLdaScore,
                sentenceInfos[i].fTotalScore,
                sentenceInfos[i].szSentence);

    }
    */

    _CLLDELETE(topDocs);
    _CLLDELETE(q);
    return true;
}
