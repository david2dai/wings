#include <iostream>
#include <fstream>
#include <string>

#include <stdlib.h>
#include <stdio.h>


#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/mman.h>  
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h> 
#include <math.h> 

#include <time.h>
#include <recsentence.h>
#include <ldainfer.h>
#include <timelog.h>
#include <string.h>
#include <vector>
#include <algorithm>

using namespace std;
void IndexSentencesWithQScore (const char * path, const char * index );
void IndexSentences (const char * path, const char * index );
void IndexSentences2 (const char * path, const char * index );
void SearchFiles (const char* index);

// float fTopicVec[SENTENCE_LDA_TOPIC_DIM];
// "/home/david/David/lda++/GibbsLDA++-0.2/models/sentences/model-final.theta"

void randomTest ();
void searchSentencesTest ();

void randomTest3 ()
{
    long MAX = 2565709L;// * sizeof (SentenceInRawData);
    FILE * infile;
    FILE * infile2;
    infile = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512_Sens.bin", "rb");
    infile2 = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512_TopicVec.bin", "rb");
    SentenceInRawData sentenceInRawData;

    vector <long> offsets;

    srand(time(0));
    for (int i=0;i<100;i++) {
        long docId = rand() % MAX;
        long offset = docId * sizeof (SentenceInRawData);
        offsets.push_back (docId);
        //cout << sentenceInRawData.szSentence << endl;

    }

    sort (offsets.begin(), offsets.end());

    long lastOffset = 0;
    fseek (infile, 0, SEEK_SET);
    TimeLog timeLog;

    for (int i=0;i<offsets.size ();i++) {
        //cout << offsets[i] << endl;
        timeLog.start ();
        fseek (infile, (offsets[i] -lastOffset) * sizeof (sentenceInRawData.szSentence) , SEEK_CUR);
        lastOffset = offsets[i];
        memset (&sentenceInRawData, 0, sizeof (SentenceInRawData));
        fread (sentenceInRawData.szSentence, sizeof (sentenceInRawData.szSentence), 1, infile);
        cout << sentenceInRawData.szSentence << endl;
        timeLog.end ("read Sentence once!");
    }

    fseek (infile2, 0, SEEK_SET);
    lastOffset = 0L;
    for (int i=0;i<offsets.size ();i++) {
        //cout << offsets[i] << endl;
        timeLog.start ();
        fseek (infile2, (offsets[i] -lastOffset) * sizeof (sentenceInRawData.fTopicVec) , SEEK_CUR);
        lastOffset = offsets[i];
        memset (&sentenceInRawData, 0, sizeof (SentenceInRawData));
        fread (sentenceInRawData.fTopicVec, sizeof (sentenceInRawData.fTopicVec), 1, infile2);
        //cout << sentenceInRawData.szSentence << endl;
        timeLog.end ("read Vector once!");
    }

    cout << endl;

    fclose (infile);
    fclose (infile2);
}

void createRandom (vector <long >& docIds) 
{
    long MAX = 2565709L; // Total sentences
    docIds.clear ();
    srand(time(0));
    
    for (int i=0;i<100;i++) {
        long docId = rand() % MAX;
        //long offset = docId * sizeof (SentenceInRawData);
        //offsets.push_back (offset);
        // offsets.push_back (docId);
        docIds.push_back (docId);
        //cout << sentenceInRawData.szSentence << endl;
    }

    //sort (offsets.begin(), offsets.end());
    sort (docIds.begin(), docIds.end());
}

void randomTest (vector <long >& docIds, vector<SentenceInRawData>& sentences)
{
    FILE * infile;
    infile = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512_.bin", "rb");
    //SentenceInRawData sentenceInRawData;

    long lastOffset = 0;
    fseek (infile, 0, SEEK_SET);

    TimeLog timeLog;

    for (int i=0;i<docIds.size ();i++) {
        //cout << offsets[i] << endl;
        //timeLog.start ();
        //cout << "Common" << endl;
        fseek (infile, (docIds[i] -lastOffset) * sizeof (SentenceInRawData) , SEEK_CUR);
        lastOffset = docIds[i] + 1;
        memset (&sentences[i], 0, sizeof (SentenceInRawData));
        fread (&sentences[i], sizeof (SentenceInRawData), 1, infile);
        //cout << sentenceInRawData.szSentence << endl;
        //timeLog.end ("read once!");
    }

    cout << endl;
    fclose (infile);
}

void randomTest_NewRead (vector <long >& docIds, vector<SentenceInRawData>& sentences)
{
    int32_t fd = ::open("/home/david/David/lucene/cluceneLib/sentencesCorpus512_.bin", O_RDONLY); //, FILEPERM);
    //int32_t fd = ::open("/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin", O_RDONLY); //, FILEPERM);
 
    ssize_t rb = 0;
    long off = 0L;

    for (int i=0;i<docIds.size ();i++) {
        memset (&sentences[i], 0, sizeof (SentenceInRawData));
 
        off = docIds[i] * sizeof (SentenceInRawData);
        long end = off + sizeof (SentenceInRawData);
        rb = ::pread(fd, &sentences[i], sizeof (SentenceInRawData), off); 
    }

    ::close(fd);
}

pthread_mutex_t mutex;
pthread_mutex_t mutex2;
int finshThreadCnt = 0;

struct ThreadInfo {
    vector <long> * offsets;
    vector<SentenceInRawData> * sentences;
    long size;
    int threadId;
};

void* threadFun(void* data)
{
    char* mapped = NULL;
    char szfile[256] = {0};
    ThreadInfo * pThreadInfo = (ThreadInfo *) data;
    
    sprintf (szfile, "/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin_%d",pThreadInfo->threadId);
    int fd = open(szfile,O_RDONLY);
    long size = pThreadInfo->size * sizeof (SentenceInRawData);

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        cout << "Error" << endl;
    }

    cout <<"sb.st_size " << sb.st_size << endl;
    
    //if ( (mapped = (char *) mmap (NULL, 4096*10, PROT_READ, MAP_SHARED, fd, 0)) == (void *)-1) 
    if ( (mapped = (char *) mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1) 
    {
        cout << "error" << endl;
    }  
   
    close(fd);

    // SentenceInRawData sentenceInRawData;
 
    long  docId = 0L;
    long  offset = 0L;
    int processCnt = 0;

    for (int i=0;i<pThreadInfo->offsets->size();i++) {
        //cout << offsets[i] << endl;
        docId = (*(pThreadInfo->offsets))[i];
        if (docId % 4 == pThreadInfo->threadId) {
            TimeLog timeLog;
            timeLog.start ();
   
            docId /= 4;
            //cout << docId << endl;
            offset = docId * sizeof (SentenceInRawData);
            
            SentenceInRawData * pData = (SentenceInRawData *) (mapped + offset);
            
            memset (&(*(pThreadInfo->sentences))[i], 0, sizeof (SentenceInRawData));
            memcpy (&(*(pThreadInfo->sentences))[i], pData, sizeof (SentenceInRawData));
            
            pthread_mutex_lock (&mutex2);
            //cout << "threadId = " <<  pThreadInfo->threadId << " process = " << docId << endl; 
            //cout << (*(pThreadInfo->sentences))[i].szSentence << endl;
            pthread_mutex_unlock (&mutex2);

            //timeLog.end ("Thread read once!");

            processCnt ++;
        }
    }

    if ((munmap((void *)mapped, size)) == -1) {
    //if ((munmap((void *)mapped, 4096*10)) == -1) {
        cout << "munmap error" << endl;
    }

    pthread_mutex_lock (&mutex);
    finshThreadCnt ++;
    //cout << "Thread Id" << pThreadInfo->threadId<< "cnt=" << processCnt << endl;
    pthread_mutex_unlock (&mutex);

}  

void randomTest_MultiThread (vector <long >& docIds, vector<SentenceInRawData>& sentences)
{
    SentenceInRawData sentenceInRawData;
    vector <long> offsets;
    TimeLog timeLog;

    ThreadInfo threadInfo[4];

    for (int i=0;i<4;i++) {
        threadInfo[i].offsets = &docIds;
        threadInfo[i].threadId = i;
        threadInfo[i].sentences = &sentences;
        threadInfo[i].size = 641427L;
    }

    threadInfo[1].size += 1;

    finshThreadCnt = 0;

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
}

/*
void searchSentencesTest ()
{

    //RecSentence  recSentence ( "./index_sentences_new", "./sentencesTopicVecBinary.bin" );
    RecSentence recSentence ("/home/david/David/lucene/cluceneLib/index_sentences_new", \
                             "/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin",\
                             "/home/david/David/lda++/GibbsLDA++-0.2/LDAModelWordInfo4Inf.bin",\
                             "/home/david/David/lucene/cluceneLib/LDAWordMapping.bin");

    vector <SentenceInfo> sentenceInfos;

    bool ret = true;
    
    ret = recSentence.searchSentencesAdvanced ("表示" , "实践" , sentenceInfos) ;
    for (int i=0;i<sentenceInfos.size ();i++) {
        cout << sentenceInfos[i].szSentence << " " << sentenceInfos[i].fLuceneScore << " " << sentenceInfos[i].fTotalScore << endl;
    }

    ret = recSentence.searchSentencesAdvanced ("关心" , "不仅" , sentenceInfos) ;
    ret = recSentence.searchSentencesAdvanced ("当时" , "决定" , sentenceInfos) ;
    ret = recSentence.searchSentencesAdvanced ("带来" , "" , sentenceInfos) ;
    ret = recSentence.searchSentencesAdvanced ("停" , "对于" , sentenceInfos) ;
    ret = recSentence.searchSentencesAdvanced ("注意" , "猪" , sentenceInfos) ;
    ret = recSentence.searchSentencesAdvanced ("总是" , "" , sentenceInfos) ;
}
*/

void FileInfo (const char* szfile) 
{
    int32_t fd = open (szfile, O_RDONLY); //, FILEPERM);
    struct stat sbuf;
    fstat (fd, &sbuf);
    cout << "File:" << szfile << endl;
    cout << "size= " << sbuf.st_size << endl;
    cout << "blksize= " << sbuf.st_blksize << endl;
    cout << "blocks= " << sbuf.st_blocks << endl;
    close (fd);

}
void creatNewSenBinLib () 
{
    FILE * infile = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin", "rb");

    SentenceInRawData * sentences = new SentenceInRawData[DOC_CNT];
    memset (sentences, 0, sizeof (SentenceInRawData) * DOC_CNT);
    fread (sentences, sizeof (SentenceInRawData) * DOC_CNT, 1, infile);
    fclose (infile);

    FILE * outfile = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512_.bin", "wb");
    
    fwrite (sentences, sizeof (SentenceInRawData) * DOC_CNT, 1, outfile);
    fclose (outfile);
    delete [] sentences;
    cout << "OK!" << endl;


}

int main(void)
{

    //cout << log (3.0) << endl;
    IndexSentencesWithQScore ("11" , "./sentences_index");
    // SearchFiles("./index_sentences");
    
    //FILE *outfile;
    //outfile = fopen("/home/david/David/tablog","at");
    //char buffer[1024] = {0};

    //searchSentencesTest ();
   
    //FileInfo ("/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin"); 
    //FileInfo ("/home/david/David/lucene/cluceneLib/sentencesCorpus512_.bin"); 
   
    //creatNewSenBinLib (); 

    // 2014.03.27 Marked
    /*    
    TimeLog timeLog;
    vector <long > docIds;
    createRandom (docIds);
    cout << sizeof (SentenceInRawData) << endl;
    const int32_t PAGESIZ = sysconf(_SC_PAGESIZE);
    cout << "page size is " << PAGESIZ << endl;
    // New Read 
    timeLog.start ();
    vector<SentenceInRawData> Senteces_new(100);
    randomTest_NewRead (docIds, Senteces_new);
    ofstream ofile_new("./result_new");
    for (int i=0;i<Senteces_new.size ();i++) {
        ofile_new << Senteces_new[i].szSentence << endl;
        //cout << Senteces_new[i].szSentence << endl;
    }
    ofile_new.close ();
    timeLog.end ("New Read");
    */
    /*
    // Multi Thread 
    timeLog.start ();
    pthread_mutex_init(&mutex,NULL);
    pthread_mutex_init(&mutex2,NULL);

    vector<SentenceInRawData> Senteces_MultiThread (100);
    randomTest_MultiThread (docIds, Senteces_MultiThread);

    ofstream ofile_MultiThread ("./result_multi");
    for (int i=0;i<Senteces_MultiThread.size ();i++) {
        ofile_MultiThread<< Senteces_MultiThread[i].szSentence << endl;
    }
    ofile_MultiThread.close ();
    timeLog.end ("Multi Thread");
    */
    /* 
    // Common read 
    timeLog.start ();
    vector<SentenceInRawData> Senteces_CommonRead (100);
    randomTest (docIds, Senteces_CommonRead);
    ofstream ofile_CommonRead("./result_common");
   
    for (int i=0;i<Senteces_CommonRead.size ();i++) {
        ofile_CommonRead << Senteces_CommonRead[i].szSentence << endl;
        cout << Senteces_CommonRead[i].szSentence << endl;
    }

    ofile_CommonRead.close ();
    timeLog.end ("Common Read");
    */
    /*
    timeLog.start ();
    searchSentencesTest ();
    timeLog.end ("Search Sentences End!");
     
    timeLog.start ();
    searchSentencesTest ();
    timeLog.end ("Search Sentences2 End!");

    */
    //sprintf(buffer,"[%s]:Get the Lda Score and Sort use time:[%lf]\n",__FILE__, dtime);
    //fwrite (buffer , strlen(buffer), 1 , outfile);
    //fflush (outfile);

    /*
    for(int i=0;i<sentenceInfos.size();i++)
    {
        cout << sentenceInfos[i].szSentence << sentenceInfos[i].fScore << endl;

        for(int j=0;j<SENTENCE_LDA_TOPIC_DIM;j++){
            cout << sentenceInfos[i].fTopicVec[j] << " ";
        }
        
        cout << endl;

    }
    */


    /*

    ifstream infileTopicVecFile ("/home/david/David/lda++/GibbsLDA++-0.2/models/sentences/model-final.theta",std::ios::in|std::ios::binary);

    FILE * outFile;
    outFile = fopen ("./testVecTopic", "wb");
    string topicVec = "";

    float fTopicVec[SENTENCE_LDA_TOPIC_DIM];
    cout << "sizeof Vector" << sizeof (fTopicVec)  << endl;
    int iStart = 0;
    int iEnd = 0;
   
    unsigned long offset = 0;
    float tmp = 0.0;

    unsigned long lsize = -1;

    cout << lsize << endl;

    for (int i=0; i< 1000 ; i++) {
        getline (infileTopicVecFile, topicVec, '\n');
        // cout << offset << endl;
       
        iStart = 0;

        for (int j=0; j< SENTENCE_LDA_TOPIC_DIM; j++){

            iEnd = topicVec.find (" ", iStart);
            tmp = atof ( topicVec.substr (iStart, iEnd - iStart).c_str () );
            ///cout << tmp << " ";
            fTopicVec[j] = tmp; //atof ( topicVec.substr (iStart, iEnd - iStart).c_str () );
            iStart = iEnd + 1;

        }

        //cout << endl;
        // fwrite(pVocabInfo, sizeof(VocabInfo),1,fout);
        fwrite (fTopicVec, sizeof (fTopicVec), 1, outFile); 

        offset += sizeof (fTopicVec);
    }

    infileTopicVecFile.close ();
    fclose (outFile);
    cout << "Save ok" << endl;


    unsigned long id = 0;
    unsigned long ulOffset = 0;

    FILE * inFile;
    inFile = fopen ( "./testVecTopic", "rb");
    float fTopicVec2[SENTENCE_LDA_TOPIC_DIM]; 

    char szTopicOffset [32] = { 0 };
    unsigned long ulSenCount = 0;
    ulSenCount = 100 ;
    sprintf(szTopicOffset, "%lu", ulSenCount * 50 * sizeof (float) ); 
    cout << "unsigned long: " << szTopicOffset << endl;
    
    ulSenCount = 3000000L;
    sprintf(szTopicOffset, "%lu", ulSenCount * 50 * sizeof (float) ); 
    cout << "unsigned long: " << szTopicOffset << endl;

    
    while ( true ) {
    
        cin >> id ;

        if (id < 1 ) {
            break;
        }

        ulOffset = (id-1) * 200;

        fseek (inFile, ulOffset , SEEK_SET);
        fread (fTopicVec2, sizeof (fTopicVec2), 1, inFile);

        for (int j=0; j< SENTENCE_LDA_TOPIC_DIM; j++) {
            cout << fTopicVec2[j] << " ";
        }

        cout << endl;

    }

    fclose (inFile);

    */

}
