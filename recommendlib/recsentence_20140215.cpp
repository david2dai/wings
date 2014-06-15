#include "recsentence.h"
#include "timelog.h"

#define _ASCII
#include "CLucene.h"

#include <iostream>
#include <string>
#include <fstream>
#include <memory.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <math.h>
#include <algorithm>
#include <map>


#include "log.h"
//#define OUTPUT_DEBUG

using namespace std;
using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::analysis::standard;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::search;
using namespace lucene::store;

pthread_mutex_t mutex;
int finshThreadCnt = 0;

struct ThreadInfo {
    vector<long>* docIds;
    vector<SentenceInRawData>* sentencesInRawData;
    long fileSize;
    int threadId;
    char fileName[512];
};

void* threadFun(void* data)
{
    // char* mapped = NULL; // David
    SentenceInRawData* mapped = NULL;
    char szMSG[256] = {0};
    ThreadInfo * pThreadInfo = (ThreadInfo *) data;
    TimeLog timeLog;
    //sprintf (szfile, "/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin_%d",pThreadInfo->threadId);
    int fd = open (pThreadInfo->fileName, O_RDONLY);
    //long size = pThreadInfo->size * sizeof (SentenceInRawData);
    
    if ( (mapped = (SentenceInRawData *) mmap (NULL, pThreadInfo->fileSize, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1) 
    {
        //cout << "error" << endl;
    }
   
    close(fd);
    // SentenceInRawData sentenceInRawData;
 
    long  docId = 0L;
    int processCnt = 0;

    timeLog.start ();

    string strTmp;
    char tmp[32] = {0};
    long lastId = 0L;
    bool first = true;
    long delta = 0L;
    long maxdelta = 0L;

    for (int i=0;i<pThreadInfo->docIds->size();i++) {
        //cout << offsets[i] << endl;
        docId = (*(pThreadInfo->docIds))[i];
        if (docId % 4 == pThreadInfo->threadId) {
   
            docId /= 4;

            if (first) {
                first = false;
                lastId = docId;
            } else {
                delta += (docId-lastId)/4;
                if ((docId-lastId)/4 > maxdelta) {
                    maxdelta =  (docId-lastId)/4;
                }

                lastId = docId;
            }

            SentenceInRawData * pData = mapped + docId;
            
            memset (&(*(pThreadInfo->sentencesInRawData))[i], 0, sizeof (SentenceInRawData));
            memcpy (&(*(pThreadInfo->sentencesInRawData))[i], pData, sizeof (SentenceInRawData));
            
            processCnt ++;
        }
    }

    float average = delta * 1.0 / processCnt;
    sprintf (szMSG, "Thread %d process %d sentences, avg delta %f,max delta %ld",
            pThreadInfo->threadId,
            processCnt,
            average,
            maxdelta);

    timeLog.end (szMSG);
    
    if ((munmap ((void *)mapped, pThreadInfo->fileSize)) == -1) {
        cout << "munmap error" << endl;
    }

    pthread_mutex_lock (&mutex);
    finshThreadCnt ++;
    //cout << "Thread Id" << pThreadInfo->threadId<< "cnt=" << processCnt << endl;
    pthread_mutex_unlock (&mutex);

}  

int cmp ( const SentenceInfo& a, const SentenceInfo& b )
{
    //if( a.fTotalScore > b.fTotalScore )  // Descending order 
    //if( a.fLuceneScore > b.fLuceneScore)  // Descending order 
    //if( a.fTotalScore < b.fTotalScore )  // Ascending order 
    if( a.fTotalScore < b.fTotalScore )  // Ascending order 
        return 1;
    else
        return 0;
}

RecSentence::RecSentence (const char * indexSen, 
                          const char * topicVecFile) 
{
    
    cd = iconv_open ("gb2312//IGNORE", "utf-8//IGNORE");
    
    analyzer = _CLNEW StandardAnalyzer ();
    indexReader = IndexReader::open (indexSen);
	indexSearcher = _CLNEW IndexSearcher (indexReader);
    
    m_ldaInfer = NULL;
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
    clock_t start, finish;
    double dtime = 0;
    start = clock();
    
    long wordId = m_ldaInfer->ldaWordId (szWord);

    if ( wordId != -1) {
        m_localContextVecWordId.push_back (wordId);
        m_localContextVecWordStr.push_back (szWord);
        while  (m_localContextVecWordId.size () > 10 ) {
            m_localContextVecWordId.erase(m_localContextVecWordId.begin());
            m_localContextVecWordStr.erase(m_localContextVecWordStr.begin());
        }

    }else {
        return false;
    }

    FILE *outfile; 
    outfile = fopen("/home/david/David/tablog","at");
    char buffer[1024] = {0};

    finish = clock();
    dtime = (double)(finish - start) / CLOCKS_PER_SEC;

    sprintf(buffer,"[%s]:add context word[%s] sucess, size if [%d] time:[%lf]\n",\
            __FILE__, szWord, m_localContextVecWordId.size (), dtime);

    fwrite (buffer , strlen(buffer), 1 , outfile);
    fclose ( outfile );

    return true;
}

/*
// Multi Thread
bool RecSentence::getSentencesFrom (std::vector<long>& docIds, 
                                    vector<SentenceInRawData>& sentencesInRawData)
{
    TimeLog timeLog;
    ThreadInfo threadInfo[4];

    for (int i=0;i<4;i++) {
        threadInfo[i].docIds = &docIds;
        threadInfo[i].sentencesInRawData = &sentencesInRawData;
        threadInfo[i].fileSize = DOC_CNT/4 * sizeof (SentenceInRawData);
        threadInfo[i].threadId = i;
        strcpy (threadInfo[i].fileName, m_SentenceCorpusFiles[i]);
    }

    for (int i=0;i<DOC_CNT%4;i++) {
        threadInfo[i].fileSize += sizeof (SentenceInRawData);
    }

    finshThreadCnt = 0;
    pthread_mutex_init (&mutex, NULL);
    
    for (int i=0;i<4;i++) {
        pthread_t pid;  
      
        if(pthread_create (&pid, NULL, threadFun, &threadInfo[i]))  
        {  
            cout << "Error!" << endl;  
        }

    }

    while (true) {
        if (4 == finshThreadCnt) {
            break;
        }
    }

    cout << "Finish!" << endl;
    return true;
}
*/

/*
// David marked
// Will use var len sentences bin file
bool RecSentence::getSentencesFrom (std::vector<long>& docIds, 
                                    vector<SentenceInRawData>& sentencesInRawData)
{
    long lastOffset = 0;
    fseek (m_sentencesFile, 0, SEEK_SET);

    for (int i=0;i<docIds.size ();++i) {
        fseek (m_sentencesFile, (docIds[i] -lastOffset) * sizeof (SentenceInRawData) , SEEK_CUR);
        lastOffset = docIds[i] + 1;
        memset (&sentencesInRawData[i], 0, sizeof (SentenceInRawData));
        fread (&sentencesInRawData[i], sizeof (SentenceInRawData), 1, m_sentencesFile);
    }

    return true;
}
*/

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
        //cout << "Sentence:id = " << senId << "[" << buffer << "]";
        
        int * tAssignsAry = (int *) (buffer + senOrgLen + 1);
        int totalWordCnt = senLdaLen / sizeof (int);
        
        /* 
        for (int j=0;j<senLdaLen/sizeof (int);++j) {
            cout << topics[j] << " ";
        }*/

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
            //cout << topicVec[j] << " ";
        }

        //cout << endl;
        delete [] buffer;
    }

    /*
    for (int i=0;i<docIds.size ();++i) {
        fseek (m_sentencesFile, (docIds[i] -lastOffset) * sizeof (SentenceInRawData) , SEEK_CUR);
        lastOffset = docIds[i] + 1;
        memset (&sentencesInRawData[i], 0, sizeof (SentenceInRawData));
        fread (&sentencesInRawData[i], sizeof (SentenceInRawData), 1, m_sentencesFile);
    }
    */

    return true;
}

const int RecSentence::REC_SENTENCES_COUNT = 200;

bool RecSentence::searchSentencesAdvanced (const char*                   szOrigWordUTF8, 
                                           const char*                   szRecWordUTF8, 
                                           std::vector <SentenceInfo> &  sentenceInfos)
{
    Log m_log ("/home/david/David/tablog");
    char queryUTF8[512] = { 0 };
    char queryGB[512] = { 0 };
   
    sprintf (queryUTF8, "%s %s", szOrigWordUTF8 , szRecWordUTF8 );
    
    if ( ! conUTF8ToGB ( queryUTF8, queryGB, sizeof (queryGB) ) )
        return false;

    Query* q = QueryParser::parse(queryGB, "sentence_gb", analyzer);
    
    SentenceInRawData sentenceInRawData;

    // start
    TopDocs* topDocs = indexSearcher->_search(q, NULL, REC_SENTENCES_COUNT);
    
    size_t scoreDocsLength = topDocs->scoreDocsLength;
    cout << scoreDocsLength << endl;

    if (scoreDocsLength == 0) {
        return false;
    }

    vector<long> docIds;
    map<long,float> scores;
    vector<SentenceInRawData> sentencesInRawData(scoreDocsLength);

    float maxScore = 0.0;
    float rawScore = 0.0;

    for (int k=0;k<scoreDocsLength && k<REC_SENTENCES_COUNT;k++) {
        long docId = topDocs->scoreDocs[k].doc;// * sizeof (SentenceInRawData);
        docIds.push_back (docId);
        rawScore = topDocs->scoreDocs[k].score;
        scores[docId] = rawScore;
        
        if (rawScore > maxScore) {
            maxScore = rawScore;
        }

    }

    /*
    Hits* h = NULL;
    h = indexSearcher->search(q);
    int iLuceneHitCnt = h->length();
    char szDocId[32] = {0};
    
    for ( size_t i=0;i<iLuceneHitCnt && i < 100;i++ ){
        
        Document* doc = &h->doc(i);
        SentenceInfo sentenceInfo;
        doc->get("doc_id") 
        sprintf (szDocId, "%s", doc->get("doc_id"));
        long docId = atol (szDocId);
        sentenceInfo.fLuceneScore = h->score(i);
        // there is a member function: int32_t id (const int32_t n); to get the id
        sentenceInfo.fLuceneScore = h->score(i);
        sentenceInfo.fLdaScore = 0.0;
        sentenceInfo.fTotalScore = 0.0;
        
        //sentenceInfos.push_back(sentenceInfo);
    }
    */

    //when use var len corpus, sort is not needed.
    //sort (docIds.begin(), docIds.end());
    getSentencesFrom (docIds, sentencesInRawData);

    for (int k=0;k<sentencesInRawData.size ();k++) {
        long docId = docIds[k];
        float fScore = scores[docId] / maxScore;
        
        SentenceInfo sentenceInfo;
        strcpy (sentenceInfo.szSentence, sentencesInRawData[k].szSentence);
        // sprintf( sentenceInfo.szSentence, "%s", sentenceInRawData.szSentence);
        sentenceInfo.fLuceneScore = fScore;
        sentenceInfo.fLdaScore = 0.0;
        sentenceInfo.fTotalScore = 0.0;
    
        memcpy (sentenceInfo.fTopicVec, sentencesInRawData[k].fTopicVec, sizeof (sentenceInfo.fTopicVec));
        /*
        for (int j=0;j<SENTENCE_LDA_TOPIC_DIM;j++) {
            sentenceInfo.fTopicVec[j] = sentenceInRawData.fTopicVec[j];
        }
        */

        sentenceInfos.push_back(sentenceInfo);
    }
        
    m_log.logInfo ("[%s][%s]:infer topic,Context size[%d]\n",
                __FILE__,
                __FUNCTION__,
                m_localContextVecWordId.size ());


    if ( m_localContextVecWordId.size () > 0 ) {
       
        // DDD 
        m_ldaInfer->inferSentence (m_localContextVecWordId, m_localContextTopicVec);
        
        string strContext;
        float minLDAScore = 10000;
        
        for (int i = 0;i < m_localContextVecWordStr.size ();i++) {

            strContext += m_localContextVecWordStr[i];
            strContext += " ";
        }

        float fMidPoint[SENTENCE_LDA_TOPIC_DIM] = {0.0};

        for (int i = 0;i < sentenceInfos.size (); i++) {

            sentenceInfos[i].fLdaScore = 0.0;

            for (int j= 0; j < SENTENCE_LDA_TOPIC_DIM; j++ ) {
                fMidPoint[j] = (sentenceInfos[i].fTopicVec[j] + m_localContextTopicVec[j]) / 2.0;
            }


            float KL_XM = calKLDistance (sentenceInfos[i].fTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            float KL_YM = calKLDistance (m_localContextTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            float KL_XY = calKLDistance (m_localContextTopicVec, sentenceInfos[i].fTopicVec, SENTENCE_LDA_TOPIC_DIM);
            //Jensen-Shannon
            float JS_Dis = 0.5 * ( KL_XM + KL_YM ); 
            
            //sentenceInfos[i].fLdaScore = exp (1.0 / (JS_Dis * 100 + 1.0 ));
            //sentenceInfos[i].fLdaScore = 1.0 / (JS_Dis * 100 + 1.0 );
            //sentenceInfos[i].fLuceneScore_ = 1.0 / ( 1.0 + exp ( -1.0 * sentenceInfos[i].fLuceneScore));
            //sentenceInfos[i].fTotalScore = sentenceInfos[i].fLdaScore * sentenceInfos[i].fLuceneScore_;
            //sentenceInfos[i].fTotalScore = KL_XY;
            // cout << "KL_XY: "<< sentenceInfos[i].fTotalScore << endl;
            
            sentenceInfos[i].fLdaScore = JS_Dis;
            //sentenceInfos[i].fTotalScore = JS_Dis; 

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
        
    sort (sentenceInfos.begin (), sentenceInfos.end (), cmp);

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

        m_log.logInfo ("[%s][%s]:SenId[%d],luceneS[%f],luceneS_[%f],ldaS[%f],total[%f],recSen[%s]\n",
                __FILE__,
                __FUNCTION__,
                docIds[i],
                sentenceInfos[i].fLuceneScore,
                sentenceInfos[i].fLuceneScore_,
                sentenceInfos[i].fLdaScore,
                sentenceInfos[i].fTotalScore,
                sentenceInfos[i].szSentence);

    }
    */

    //_CLLDELETE(h);
    _CLLDELETE(topDocs);
    _CLLDELETE(q);
    
    return true;
}

bool RecSentence::searchSentencesAdvanced2 (const char*                   szOrigWordUTF8, 
                                           const char*                   szRecWordUTF8, 
                                           std::vector <SentenceInfo> &  sentenceInfos)
{
    
    #ifdef OUTPUT_DEBUG
    TimeLog timeLog_1;
    TimeLog timeLog_2;
    timeLog_1.start ();
    #endif
        
    char queryUTF8[512] = { 0 };
    char queryGB[512] = { 0 };
   
    sprintf (queryUTF8, "\"%s\" \"%s\"", szOrigWordUTF8 , szRecWordUTF8 );
    
    if ( ! conUTF8ToGB ( queryUTF8, queryGB, sizeof (queryGB) ) )
        return false;


    #ifdef OUTPUT_DEBUG
    timeLog_2.start ();
    #endif

    Query* q = QueryParser::parse(queryGB, "sentence_gb", analyzer);
    Hits* h = NULL;
    //h = indexSearcher->search(q);
       
    string strTopicVec = "";
    string strTopicOffset  = "";
    unsigned long ulTopicOffset  = 0;

    string strSub = "";
    int iStart = 0;
    int iEnd = 0;


    /*
    int iLuceneHitCnt = h->length();
    
    for ( size_t i=0;i<iLuceneHitCnt && i < 500;i++ ){
        
        Document* doc = &h->doc(i);
        SentenceInfo sentenceInfo;
       
        sprintf( sentenceInfo.szSentence, "%s", doc->get("sentence_utf") );
        sentenceInfo.fLuceneScore = h->score(i);
        sentenceInfo.fLdaScore = 0.0;
        sentenceInfo.fTotalScore = 0.0;
        
        // topic_vec_offset
        // cout << i << "\t" << "[" << sentenceInfo.szSentence << "]\t" << sentenceInfo.fScore << endl ;
        // cout << strTopicVec << endl;

        // David need update the following

        strTopicVec = doc->get("topic_vec");
        
        iStart = 0;
        for (int j=0;j<SENTENCE_LDA_TOPIC_DIM;j++) {
            iEnd = strTopicVec.find (" ", iStart);
            strSub = strTopicVec.substr(iStart, iEnd-iStart) ;
            sentenceInfo.fTopicVec[j] = atof( strSub.c_str() );
            // sentenceInfo.fTopicVec[j] = atof( strSub.substr(strSub.find(":") + 1).c_str() );
            //cout << strSub.substr(strSub.find(":")).c_str() << " ";
            //cout << sentenceInfo.fTopicVec[j] << " ";
            iStart = iEnd + 1;
        }

        #ifdef OUTPUT_DEBUG
        sprintf(buffer,"[%s]:luceneS[%f],luceneS_[%f],ldaS[%f],total[%f],recSen[%s]\n",\
                __FILE__, \
                sentenceInfo.fLuceneScore,\
                sentenceInfo.fLuceneScore_,\
                sentenceInfo.fLdaScore,\
                sentenceInfo.fTotalScore,\
                sentenceInfo.szSentence);

        fwrite (buffer , strlen(buffer), 1 , outfile);
        fflush (outfile);
        #endif

        
        sentenceInfos.push_back(sentenceInfo);
        //cout <<endl;
    }

    */

    TopDocs* topDocs = indexSearcher->_search(q, NULL, 500);
    //topDocs->scoreDocs;
    #ifdef OUTPUT_DEBUG
    timeLog_2.end ("searchSentencesAdvanced Step 1:New Search");
    timeLog_2.start ();
    #endif

    size_t scoreDocsLength = topDocs->scoreDocsLength;
    cout << scoreDocsLength << endl;

    for (int k=0;k<scoreDocsLength;k++) {
        //Document firstHit = searcher.doc(rs.scoreDocs[i].doc); 
        Document * doc= indexSearcher->doc(topDocs->scoreDocs[k].doc); 
        //cout << doc->get("sentence_utf") <<endl;
        //cout << topDocs->scoreDocs[k].doc << " " << topDocs->scoreDocs[k].score<< endl;

        SentenceInfo sentenceInfo;
       
        sprintf( sentenceInfo.szSentence, "%s", doc->get("sentence_utf") );
        //sentenceInfo.fLuceneScore = h->score(i);
        sentenceInfo.fLuceneScore = topDocs->scoreDocs[k].score;
        sentenceInfo.fLdaScore = 0.0;
        sentenceInfo.fTotalScore = 0.0;
        
        strTopicVec = doc->get("topic_vec");
        
        iStart = 0;
        for (int j=0;j<SENTENCE_LDA_TOPIC_DIM;j++) {
            iEnd = strTopicVec.find (" ", iStart);
            strSub = strTopicVec.substr(iStart, iEnd-iStart) ;
            sentenceInfo.fTopicVec[j] = atof( strSub.c_str() );
            iStart = iEnd + 1;
        }

        sentenceInfos.push_back(sentenceInfo);
    }


    #ifdef OUTPUT_DEBUG
    timeLog_2.end ("searchSentencesAdvanced Step 2:Get Hits");
    timeLog_2.start ();
    #endif

    //if (m_localContextVecWordId.size () > 5 ) {
    if (1 || m_localContextVecWordId.size () > 5 ) {
        

        string strContext;
        
        for (int i = 0;i < m_localContextVecWordStr.size ();i++) {

            strContext += m_localContextVecWordStr[i];
            strContext += " ";
        }

        m_ldaInfer->inferSentence (m_localContextVecWordId, m_localContextTopicVec);

        float fMidPoint[SENTENCE_LDA_TOPIC_DIM] = {0.0};

        for (int i = 0;i < sentenceInfos.size (); i++) {

            sentenceInfos[i].fLdaScore = 0.0;

            for (int j= 0; j < SENTENCE_LDA_TOPIC_DIM; j++ ) {
                fMidPoint[j] = (sentenceInfos[i].fTopicVec[j] + m_localContextTopicVec[j]) / 2.0;
            }

            float KL_XM = calKLDistance (sentenceInfos[i].fTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
            float KL_YM = calKLDistance (m_localContextTopicVec, fMidPoint, SENTENCE_LDA_TOPIC_DIM);
           
            float KL_XY = calKLDistance (m_localContextTopicVec, sentenceInfos[i].fTopicVec, SENTENCE_LDA_TOPIC_DIM);
            //Jensen-Shannon
            float JS_Dis = 0.5 * ( KL_XM + KL_YM ); 
            
            //sentenceInfos[i].fLdaScore = exp (1.0 / (JS_Dis * 100 + 1.0 ));
            sentenceInfos[i].fLdaScore = 1.0 / (JS_Dis * 100 + 1.0 );
            sentenceInfos[i].fLuceneScore_ = 1.0 / ( 1.0 + exp ( -1.0 * sentenceInfos[i].fLuceneScore));
            //sentenceInfos[i].fTotalScore = sentenceInfos[i].fLdaScore * sentenceInfos[i].fLuceneScore_;
            sentenceInfos[i].fTotalScore = KL_XY;
            // cout << "KL_XY: "<< sentenceInfos[i].fTotalScore << endl;
        
        }
    
        sort (sentenceInfos.begin (), sentenceInfos.end (), cmp);

        #ifdef OUTPUT_DEBUG
        /* 
        for (int i= 0;i < sentenceInfos.size (); i++) {

            sprintf(buffer,"[%s]:luceneS[%f],luceneS_[%f],ldaS[%f],total[%f],recSen[%s]\n",\
                    __FILE__, \
                    sentenceInfos[i].fLuceneScore,\
                    sentenceInfos[i].fLuceneScore_,\
                    sentenceInfos[i].fLdaScore,\
                    sentenceInfos[i].fTotalScore,\
                    sentenceInfos[i].szSentence);

            fwrite (buffer , strlen(buffer), 1 , outfile);
            fflush (outfile);
        }
        */
        #endif

    }
        
    #ifdef OUTPUT_DEBUG
    timeLog_2.end ("searchSentencesAdvanced Step 3:LDA and Sort");

    char szBuff[512] = {0};
    sprintf(szBuff,"[%s]:searchSentencesAdvanced:Orig[%s]Rec[%s]",__FILE__, szOrigWordUTF8,szRecWordUTF8);
    timeLog_1.end (szBuff);
    #endif
    _CLLDELETE(h);
    _CLLDELETE(topDocs);
    _CLLDELETE(q);

    return true;
}


bool RecSentence::searchSentences (const char * szOrigWordUTF8 , 
                                   const char * szRecWordUTF8 , 
                                   vector <SentenceInfo> & sentenceInfos)
{
    /*
    char szOrigWordUTF8_[256] = { 0 };
    char szRecWordUTF8_[256] = { 0 };
    char szOrigWordGB[256] = { 0 };
    char szRecWordGB[256] = { 0 };
    */

    char queryUTF8[512] = { 0 };
    char queryGB[512] = { 0 };
   
    sprintf (queryUTF8, "\"%s\" \"%s\"", szOrigWordUTF8 , szRecWordUTF8 );
    
    if ( ! conUTF8ToGB ( queryUTF8, queryGB, sizeof (queryGB) ) )
        return false;
    
    Query* q = QueryParser::parse(queryGB, "sentence_gb", analyzer);

    // debug
    /*
    char * buf;
    buf = q->toString();
    printf("Searching for: %s\n\n", buf);
    _CLDELETE_LCARRAY(buf);
    */

    Hits* h = indexSearcher->search(q);
    string strTopicVec = "";
    string strSub = "";
    
    string strTopicOffset  = "";
    unsigned long ulTopicOffset  = 0;

    int iStart = 0;
    int iEnd = 0;
    

    for ( size_t i=0;i<h->length() && i < 100;i++ ){
        Document* doc = &h->doc(i);
        SentenceInfo sentenceInfo;
        sprintf( sentenceInfo.szSentence, "%s", doc->get("sentence_utf") );
        sentenceInfo.fLuceneScore = h->score(i);
        strTopicVec = doc->get("topic_vec");
        // topic_vec_offset
        //cout << i << "\t" << "[" << sentenceInfo.szSentence << "]\t" << sentenceInfo.fScore << endl ;
        // cout << strTopicVec << endl;

        // David need update the following
        /*
        strTopicOffset = doc->get("topic_vec_offset"); 
        cout << strTopicOffset << endl;
        ulTopicOffset = atol ( strTopicOffset.c_str () ); 
        fseek (m_TopicVecFile, ulTopicOffset, SEEK_SET);
        fread (sentenceInfo.fTopicVec, sizeof (sentenceInfo.fTopicVec), 1, m_TopicVecFile);
        */

        // OK END
        //iEnd = strTopicVec.find ("\t", iStart);
        //iStart = iEnd + 1;

        for (int j=0;j<SENTENCE_LDA_TOPIC_DIM;j++) {
            iEnd = strTopicVec.find (" ", iStart);
            strSub = strTopicVec.substr(iStart, iEnd-iStart) ;
            sentenceInfo.fTopicVec[j] = atof( strSub.c_str() );
            // sentenceInfo.fTopicVec[j] = atof( strSub.substr(strSub.find(":") + 1).c_str() );
            //cout << strSub.substr(strSub.find(":")).c_str() << " ";
            //cout << sentenceInfo.fTopicVec[j] << " ";
            iStart = iEnd + 1;
        }

        // David need update END

        sentenceInfos.push_back(sentenceInfo);
        //cout <<endl;
    }

    _CLLDELETE(h);
    _CLLDELETE(q);

    return true;
}
