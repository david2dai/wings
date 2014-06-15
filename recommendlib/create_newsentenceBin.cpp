#include "recsentence.h"
#include "timelog.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <algorithm>

//for mmap
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ldainfer.h"
#include "cachefile.h"

using namespace std;

//sentencesCorpus512.bin format is as bellow
struct SentenceInRawData_Old {
    char szSentence[256]; // sentence max length is 150
    float fTopicVec[50];
    float qscore;
    char szPadding[256-50*4-sizeof (float)];
    // 256 + 200 + 4 + 256-200 -4 = 512
};


void useNewSentencesBinFile_mmap () {


    TimeLog timeLog;
    cout << "DOC count is " << DOC_CNT << endl;
    
    timeLog.start ();
    FILE * infile_offset = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpusOffset.bin", "rb");
    SentenceOffset * sentencesOffset = new SentenceOffset[DOC_CNT];
    memset (sentencesOffset, 0, sizeof (SentenceOffset) * DOC_CNT);
    fread (sentencesOffset, sizeof (SentenceOffset) * DOC_CNT, 1, infile_offset);
    fclose (infile_offset);
  
    timeLog.end ("Load offset BinFinle!");
    
    cout << "Finish load offset" << endl;
    
    char* mapped = NULL;
    int fd = open("/home/david/David/lucene/cluceneLib/sentencesCorpusMain.bin", O_RDONLY);
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        cout << "Error" << endl;
    }

    cout <<"sb.st_size " << sb.st_size << endl;
    size_t fsize = sb.st_size;
    if ( (mapped = (char *) mmap (NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1) 
    {
        cout << "error" << endl;
    }  
     
    close(fd);

    vector<long> senIds;
    srand(time(0));
    long senId = 0;

    for (int i=0;i<200;i++) {
        senId = rand() % DOC_CNT;
        senIds.push_back (senId);
        //cout << sentenceInRawData.szSentence << endl;
    }

    cout << "Finish gen rand senId" << endl;

    timeLog.start ();
    sort (senIds.begin(), senIds.end());

    long offset = 0;
    int senOrgLen = 0;
    int senLdaLen = 0;
    int senInfoDataLen = 0;

    float qscore = 0.0;
    SentenceInRawData_Old  sentenceInRawData;
    long lastOffset = 0;

    for (int i=0;i<senIds.size ();++i) {
        senId = senIds[i];
        offset = sentencesOffset[senId].offset;
        senOrgLen = sentencesOffset[senId].orgLen;
        senLdaLen = sentencesOffset[senId].ldaLen;

        senInfoDataLen =  senOrgLen + 1 + sizeof (qscore)  + senLdaLen;
        
        char * buffer = new char [senInfoDataLen];
        //fseek (infile_main, offset, SEEK_SET);
        memcpy (buffer, mapped+offset, senInfoDataLen);
        cout << "Sentence:id = " << senId << "[" << buffer << "]";
        strcpy (sentenceInRawData.szSentence, buffer); 
        sentenceInRawData.qscore = sentencesOffset[senId].qScore/10.0;
        //cout << " qscore=" << sentenceInRawData.qscore << " ";
        unsigned char * topics = (unsigned char *) (buffer + senOrgLen + 1 );
       
        /* 
        for (int j=0;j<senLdaLen/sizeof (unsigned char);++j) {
            cout << (int)topics[j] << " ";
        }
        */

        int totalWordCnt = senLdaLen/sizeof (int);

        float alpha = 1.0;
        int tAssignCnt = 0; 
        float topicVec[TOPIC_DIM];

        for (int j=0,k=0;j<TOPIC_DIM;++j) {
            tAssignCnt = 0; 
            while ( j == topics[k]  && k< totalWordCnt) {
                ++k;
                ++tAssignCnt;
            }

            topicVec[j] = (tAssignCnt + alpha) / (totalWordCnt + alpha * TOPIC_DIM);
        }

        cout << endl;
        delete [] buffer;
    }

    if ((munmap((void *)mapped, fsize)) == -1) {
    //if ((munmap((void *)mapped, 4096*10)) == -1) {
        cout << "munmap error" << endl;
    }
    
    timeLog.end ("Common->GetSentences!");
}

void useNewSentencesBinFile_Cache () {

    TimeLog timeLog;
    cout << "DOC count is " << DOC_CNT << endl;
    
    timeLog.start ();
    FILE * infile_offset = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpusOffset.bin", "rb");
    SentenceOffset * sentencesOffset = new SentenceOffset[DOC_CNT];
    memset (sentencesOffset, 0, sizeof (SentenceOffset) * DOC_CNT);
    fread (sentencesOffset, sizeof (SentenceOffset) * DOC_CNT, 1, infile_offset);
    fclose (infile_offset);
  
    timeLog.end ("Load offset BinFinle!");
    
    cout << "Finish load offset" << endl;
   
    int bufsize = 64; 
    CacheFile * cacheFile = new CacheFile ("/home/david/David/lucene/cluceneLib/sentencesCorpusMain.bin", bufsize);
    
    vector<long> senIds;
    srand(time(0));
    long senId = 0;

    for (int i=0;i<200;i++) {
        senId = rand() % DOC_CNT;
        senIds.push_back (senId);
        //cout << sentenceInRawData.szSentence << endl;
    }

    cout << "Finish gen rand senId" << endl;

    timeLog.start ();
    sort (senIds.begin(), senIds.end());

    long offset = 0;
    int senOrgLen = 0;
    int senLdaLen = 0;
    int senInfoDataLen = 0;

    float qscore = 0.0;
    SentenceInRawData_Old  sentenceInRawData;
    cacheFile->seekBegin ();
    for (int i=0;i<senIds.size ();++i) {

        senId = senIds[i];
        offset = sentencesOffset[senId].offset;
        senOrgLen = sentencesOffset[senId].orgLen;
        senLdaLen = sentencesOffset[senId].ldaLen;

        senInfoDataLen =  senOrgLen + 1 + sizeof (qscore)  + senLdaLen;
        char * buffer = new char [senInfoDataLen];
       
        cacheFile->read(buffer, offset, senInfoDataLen); 
        
        cout << "Sentence:id = " << senId << "[" << buffer << "]";
        strcpy (sentenceInRawData.szSentence, buffer); 
        sentenceInRawData.qscore = sentencesOffset[senId].qScore/10.0;
        //cout << " qscore=" << sentenceInRawData.qscore << " ";
        unsigned char * topics = (unsigned char *) (buffer + senOrgLen + 1 );
       
        /* 
        for (int j=0;j<senLdaLen/sizeof (unsigned char);++j) {
            cout << (int)topics[j] << " ";
        }
        */

        int totalWordCnt = senLdaLen/sizeof (int);

        float alpha = 1.0;
        int tAssignCnt = 0; 
        float topicVec[TOPIC_DIM];

        for (int j=0,k=0;j<TOPIC_DIM;++j) {
            tAssignCnt = 0; 
            while ( j == topics[k]  && k< totalWordCnt) {
                ++k;
                ++tAssignCnt;
            }

            topicVec[j] = (tAssignCnt + alpha) / (totalWordCnt + alpha * TOPIC_DIM);
        }

        //cout << endl;
        delete [] buffer;
    }
    delete cacheFile;
    cacheFile = NULL;
    char szMSG[128] = {0};
    sprintf(szMSG, "CacheFile[bufsize=%d]->GetSentences!", bufsize);
    timeLog.end (szMSG);
}

void useNewSentencesBinFile () {

    TimeLog timeLog;
    cout << "DOC count is " << DOC_CNT << endl;
    
    timeLog.start ();
    FILE * infile_offset = fopen ("/home/david/David/lucene/cluceneLib/testfile/sentencesCorpusOffset.bin", "rb");
    SentenceOffset * sentencesOffset = new SentenceOffset[DOC_CNT];
    memset (sentencesOffset, 0, sizeof (SentenceOffset) * DOC_CNT);
    fread (sentencesOffset, sizeof (SentenceOffset) * DOC_CNT, 1, infile_offset);
    fclose (infile_offset);
  
    timeLog.end ("Load offset BinFinle!");
    
    cout << "Finish load offset" << endl;
    
    FILE * infile_main = fopen ("/home/david/David/lucene/cluceneLib/testfile/sentencesCorpusMain3.bin", "rb");
     
    vector<long> senIds;
    srand(time(0));
    long senId = 0;

    for (int i=0;i<200;i++) {
        senId = rand() % DOC_CNT;
        senIds.push_back (senId);
        //cout << sentenceInRawData.szSentence << endl;
    }

    cout << "Finish gen rand senId" << endl;


    timeLog.start ();
    sort (senIds.begin(), senIds.end());

    long offset = 0;
    int senOrgLen = 0;
    int senLdaLen = 0;
    int senInfoDataLen = 0;

    float qscore = 0.0;
    SentenceInRawData_Old  sentenceInRawData;
    long lastOffset = 0;

    for (int i=0;i<senIds.size ();++i) {

        senId = senIds[i];
        offset = sentencesOffset[senId].offset;
        senOrgLen = sentencesOffset[senId].orgLen;
        senLdaLen = sentencesOffset[senId].ldaLen;

        senInfoDataLen =  senOrgLen + 1 + sizeof (qscore)  + senLdaLen;
        char * buffer = new char [senInfoDataLen];
        //fseek (infile_main, offset, SEEK_SET);
        fseek (infile_main, offset-lastOffset, SEEK_CUR);
        fread (buffer, senInfoDataLen, 1, infile_main);
        lastOffset = offset + senInfoDataLen; 
        cout << "Sentence:id = " << senId << "[" << buffer << "]";
        strcpy (sentenceInRawData.szSentence, buffer); 
        sentenceInRawData.qscore = sentencesOffset[senId].qScore/10.0;
        //cout << " qscore=" << sentenceInRawData.qscore << " ";
        unsigned char * topics = (unsigned char *) (buffer + senOrgLen + 1 );
       
        /* 
        for (int j=0;j<senLdaLen/sizeof (unsigned char);++j) {
            cout << (int)topics[j] << " ";
        }
        */

        int totalWordCnt = senLdaLen/sizeof (int);

        float alpha = 1.0;
        int tAssignCnt = 0; 
        float topicVec[TOPIC_DIM];

        for (int j=0,k=0;j<TOPIC_DIM;++j) {
            tAssignCnt = 0; 
            while ( j == topics[k]  && k< totalWordCnt) {
                ++k;
                ++tAssignCnt;
            }

            topicVec[j] = (tAssignCnt + alpha) / (totalWordCnt + alpha * TOPIC_DIM);
        }

        cout << endl;
        delete [] buffer;
    }

    fclose (infile_main);
    timeLog.end ("Common->GetSentences!");
}

void createNewSentencesBinFile () {
    FILE * infile = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin", "rb");

    SentenceInRawData_Old * sentences = new SentenceInRawData_Old[DOC_CNT];
    memset (sentences, 0, sizeof (SentenceInRawData_Old) * DOC_CNT);
    fread (sentences, sizeof (SentenceInRawData_Old) * DOC_CNT, 1, infile);
    fclose (infile);

    cout << "Finish read 512 bin" << endl;

    FILE * outfile_main = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpusMain.bin", "wb");
    FILE * outfile_offset = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpusOffset.bin", "wb");
    int senOrgLen = 0;
    int senLdaLen = 0;
    int senInfoDataLen = 0;

    ifstream fileSenZAssigan ("/home/david/David/lda++/GibbsLDA++-0.2/models/sentencesFinal/model-final.tassign_new");
    string line;
    
    ifstream fileSenQScore ("/home/david/David/rank/data/predictAllSentencesFinalScore");
    string strQScore;
    float qscore = 0.0;

    long offset = 0;
    SentenceOffset sentenceOffset;
    char * pbuf = NULL;

    int iQScore = 0;
    int maxLen = 0;

    for (long i=0;i<DOC_CNT;++i) {

        senOrgLen = strlen (sentences[i].szSentence);
        if (senOrgLen > maxLen ) {
            maxLen = senOrgLen;
        }

        getline (fileSenZAssigan, line, '\n');
        getline (fileSenQScore, strQScore, '\n');

        qscore = atof (strQScore.c_str ());
        iQScore = qscore * 10;
        int start = 0;
        int end = 0;
        string sub;
        int topicId =0;
        vector<int> topics;

        while ( (end = line.find (" ", start)) != -1) {
            sub = line.substr (start, end-start);
            start = end + 1;
             
            topicId = atoi (sub.c_str ());
            //cout << topicId << " ";
            topics.push_back (topicId);

        }

        sort (topics.begin (), topics.end ());
        
        // topic id length 
        //senLdaLen = topics.size () * sizeof (int);
        senLdaLen = topics.size () * sizeof (unsigned char);
       
         
        //senInfoDataLen =  senOrgLen + 1 + sizeof (qscore) + senLdaLen;

        senInfoDataLen =  senOrgLen + 1 + senLdaLen;
        
        char * buffer = new char[senInfoDataLen]; // Modify
        //int * topicAry = new int[topics.size ()];
        unsigned char * topicAry = new unsigned char[topics.size ()];

        // move topic id to int ary 
        for (int j=0;j<topics.size ();++j) {
            //cout << topics[j] << " ";
            topicAry[j] = topics[j];
        }

        // cpy sentence to buffer
        strcpy (buffer, sentences[i].szSentence);
        pbuf = buffer + senOrgLen + 1;
        //memcpy (pbuf, &qscore, sizeof (qscore)); // Add
        //pbuf = pbuf + sizeof (qscore);
        memcpy (pbuf, topicAry, senLdaLen);  // Modify

        sentenceOffset.offset = offset;
        sentenceOffset.orgLen = senOrgLen;
        sentenceOffset.ldaLen = senLdaLen;
        sentenceOffset.qScore = iQScore;

        //offset += senOrgLen + 1;
        //offset += sizeof (float); // Add
        //offset += senLdaLen;
        offset += senInfoDataLen;
        
        fwrite (buffer, senInfoDataLen, 1, outfile_main); // Modify
        fwrite (&sentenceOffset, sizeof (SentenceOffset), 1, outfile_offset);

    
        delete [] buffer;
        delete [] topicAry;
        /*
        cout << "Sen = ["<< sentences[i].szSentence << "] Lda=[" << line << "]"<< endl;
        */
    }

    fileSenZAssigan.close (); 
    fileSenQScore.close (); 
    fclose (outfile_main);
    fclose (outfile_offset);
    delete [] sentences;
    cout << "Max sentence length is " << maxLen << endl;  //150
    cout << "OK!" << endl;

}

int main (void) {
    //createNewSentencesBinFile ();
    useNewSentencesBinFile_mmap (); 
    //useNewSentencesBinFile_Cache ();
    //useNewSentencesBinFile ();
    //cout << sizeof (SentenceOffset) << endl;
}


