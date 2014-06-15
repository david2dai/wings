#define _ASCII //#define _MBCS
#include "CLucene.h"

//#include "CLucene/StdHeader.h"
//#include "CLucene/_clucene-config.h"

/*
#include "CLucene/util/CLStreams.h"
#include "CLucene/util/dirent.h"
#include "CLucene/config/repl_tchar.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/StringBuffer.h"
*/
#include <iostream>
#include <string>
#include <fstream>
#include <memory.h>
#include <math.h>

#include <recsentence.h>

using namespace std;

using namespace lucene::index;
using namespace lucene::analysis;
//using namespace lucene::util;
//using namespace lucene::store;
using namespace lucene::document;


// 2014.03.27  Add Sentence Quality  Score
void IndexSentencesWithQScore (const char * path, const char * index )
{
    ifstream infileGBSegFile ("./finaldata/SensFilteredInvalidGB_Seg_Fileter11InvalidLDASens.dat",
            std::ios::in|std::ios::binary);
    ifstream infileQScore ("/home/david/David/rank/data/predictAllSentencesFinalScore",
            std::ios::in|std::ios::binary);

    IndexWriter * writer = NULL;
    standard::StandardAnalyzer an;
	writer = _CLNEW IndexWriter(index, &an, true);
    // To bypass a possible exception (we have no idea what we will be indexing...)
    writer->setMaxFieldLength(0x7FFFFFFFL); // LUCENE_INT32_MAX_SHOULDBE
    // Turn this off to make indexing faster; we'll turn it on later before optimizing
    //writer->setUseCompoundFile(false);
    writer->setUseCompoundFile(true);
    //writer->setMaxBufferedDocs(100);
    //writer->setMergeFactor(100);

    string senGB;
    string senQScore;
    
    unsigned long ulSenCount = 0;
    int maxLen = 0;
    float boost = 0.0;
    
    while( getline(infileGBSegFile, senGB,'\n') && getline(infileQScore, senQScore,'\n') ) {
            
            if (ulSenCount % 10000 == 0){
                cout << "Sentence:" << ulSenCount <<endl;
            }
            
            //if (lSenCount > 100000)
            //cout << "Sentence:" << lSenCount <<" [" <<  senGB.c_str() << "]"  <<endl;
            
            //doc.add( *_CLNEW Field("sentence_utf", senUTF8.c_str(), Field::STORE_YES | Field::INDEX_NO  ) );
            //doc.add( *_CLNEW Field("topic_vec",    topicVec.c_str(), Field::STORE_YES | Field::INDEX_NO ) );
           
            // Store topic vec into another binary file 
            //memset (szTopicOffset, 0, sizeof (szTopicOffset) );
            //sprintf (szTopicOffset, "%lu", ulSenCount * SENTENCE_LDA_TOPIC_DIM * sizeof (float) ); 
            
            int temp = senQScore.find ("\n");
            
            if ( temp >=0) {
                cout << "Error!" << endl;
            }
    
            Document doc;
            Field * senGbField = _CLNEW Field("sentence_gb",  senGB.c_str() , Field::STORE_NO | Field::INDEX_TOKENIZED);
            boost = atof (senQScore.c_str());
            boost = log (1.0 + boost);
            cout<< boost << endl;
            senGbField->setBoost (boost);
            doc.add (*senGbField);
            //doc.add( *_CLNEW Field("sentence_gb",  senGB.c_str() , Field::STORE_NO | Field::INDEX_TOKENIZED) );
            //doc.add( *_CLNEW Field("qscore",  senQScore.c_str() , Field::STORE_YES | Field::INDEX_NO) );
            
            writer->addDocument( &doc );
            
            ++ ulSenCount;
    }

    cout << "SenCount:2567948==" <<ulSenCount << endl;
    // Make the index use as little files as possible, and optimize it
   
    writer->optimize();
    writer->close();
	_CLLDELETE(writer);
   
    infileGBSegFile.close ();
    infileQScore.close ();

    cout << "Max sentence len" << maxLen << endl;	
    cout << "Finish & Close" << endl;	
	// printf("Indexing took: %d ms.\n\n", (int32_t)(Misc::currentTimeMillis() - str));
}


/*

// Final Create Index
void IndexSentences2 (const char * path, const char * index )
{
      
    int i = 0;
    //#define test 
    #ifdef test 
    ifstream infileGBSegFile ("./rawdata/segSenGB_afterSeg",std::ios::in|std::ios::binary);
    ifstream infileUTF8File ("./rawdata/segSenUTF8",std::ios::in|std::ios::binary);
    ifstream infileTopicVecFile ("./rawdata/topicsMTest",std::ios::in|std::ios::binary);
    ifstream infileClusterFile ("./rawdata/skResultTest",std::ios::in|std::ios::binary);
    #else
    //test remove stop word speed
    //ifstream infileGBSegFile ("/home/david/David/corpus2.0/sentencesLib/ForLuceneIndex_WithOutStopWords",std::ios::in|std::ios::binary);
    ifstream infileGBSegFile ("./finaldata/SensFilteredInvalidGB_Seg_Fileter11InvalidLDASens.dat",std::ios::in|std::ios::binary);
    ifstream infileUTF8File ("./finaldata/SensFilteredInvalidUTF8_Fileter11InvalidLDASens.dat",std::ios::in|std::ios::binary);
    ifstream infileTopicVecFile ("/home/david/David/lda++/GibbsLDA++-0.2/models/sentencesFinal/model-final.theta",std::ios::in|std::ios::binary);
    //ifstream infileGBSegFile ("./finaldata/SensFilteredInvalidGB_Seg.dat",std::ios::in|std::ios::binary);
    //ifstream infileUTF8File ("./finaldata/SensFilteredInvalidUTF8.dat",std::ios::in|std::ios::binary);
    //ifstream infileTopicVecFile ("./finaldata/topicsMatrix.txt_simple",std::ios::in|std::ios::binary);
    ifstream infileClusterFile ("./finaldata/beforeRemoveLDAInvalidSens/kMeansResult_20.txt",std::ios::in|std::ios::binary); // damy
    #endif

   
    IndexWriter * writer = NULL;
    standard::StandardAnalyzer an;
	writer = _CLNEW IndexWriter(index, &an, true);
    // To bypass a possible exception (we have no idea what we will be indexing...)
    writer->setMaxFieldLength(0x7FFFFFFFL); // LUCENE_INT32_MAX_SHOULDBE
    // Turn this off to make indexing faster; we'll turn it on later before optimizing
    //writer->setUseCompoundFile(false);
    writer->setUseCompoundFile(true);
    //writer->setMaxBufferedDocs(100);
    //writer->setMergeFactor(100);


    string senGB;
    string senUTF8;
    string topicVec;
    string cluster;
    
    unsigned long ulSenCount = 0;
    char szSentenceRawDataOffset [32] = { 0 };
    int iStart = 0;
    int iEnd = 0;
    
    //FILE * outTopicVecBinaryFile;
    FILE * outTopicVecBinaryFile[4];
    outTopicVecBinaryFile[0] = fopen ("./sentencesCorpus512.bin_0", "wb");
    outTopicVecBinaryFile[1] = fopen ("./sentencesCorpus512.bin_1", "wb");
    outTopicVecBinaryFile[2] = fopen ("./sentencesCorpus512.bin_2", "wb");
    outTopicVecBinaryFile[3] = fopen ("./sentencesCorpus512.bin_3", "wb");
    //outTopicVecBinaryFile_vec = fopen ("./sentencesCorpus512_TopicVec.bin", "wb");
    SentenceInRawData sentenceInRawData;   // defined in recsentence.h
   
    int maxLen = 0;
    
    while( getline(infileGBSegFile, senGB,'\n') && \
           getline(infileUTF8File, senUTF8,'\n') && \
           getline(infileTopicVecFile, topicVec,'\n') && \
           getline(infileClusterFile, cluster,'\n')  ) {
            
            if (ulSenCount % 10000 == 0){
                cout << "Sentence:" << ulSenCount <<endl;
            }
            //if (lSenCount > 100000)
            //cout << "Sentence:" << lSenCount <<" [" <<  senGB.c_str() << "]"  <<endl;
            
            //doc.add( *_CLNEW Field("sentence_utf", senUTF8.c_str(), Field::STORE_YES | Field::INDEX_NO  ) );
            //doc.add( *_CLNEW Field("topic_vec",    topicVec.c_str(), Field::STORE_YES | Field::INDEX_NO ) );
           
            // Store topic vec into another binary file 
            //memset (szTopicOffset, 0, sizeof (szTopicOffset) );
            //sprintf (szTopicOffset, "%lu", ulSenCount * SENTENCE_LDA_TOPIC_DIM * sizeof (float) ); 
            
            memset (&sentenceInRawData, 0, sizeof (SentenceInRawData));
            sprintf (szSentenceRawDataOffset, "%lu", ulSenCount * sizeof (SentenceInRawData) ); 
            strcpy (sentenceInRawData.szSentence, senUTF8.c_str ());

            int len = strlen (sentenceInRawData.szSentence);
            if (len > maxLen) {
                maxLen = len;
            }

            sentenceInRawData.clusterID = 1;
           
            iStart = 0;

            for (int j=0; j< SENTENCE_LDA_TOPIC_DIM; j++){

                iEnd = topicVec.find (" ", iStart);
                float tmp = atof ( topicVec.substr (iStart, iEnd - iStart).c_str () );
                //fTopicVec[j] = tmp; //atof ( topicVec.substr (iStart, iEnd - iStart).c_str () );
                sentenceInRawData.fTopicVec[j] = tmp;
                iStart = iEnd + 1;

            }


            int mapId = ulSenCount % 4;
            fwrite (&sentenceInRawData, sizeof (SentenceInRawData), 1, outTopicVecBinaryFile[mapId]); 
            //fwrite (sentenceInRawData.szSentence, sizeof (sentenceInRawData.szSentence), 1, outTopicVecBinaryFile); 
            //fwrite (sentenceInRawData.fTopicVec, sizeof (sentenceInRawData.fTopicVec), 1, outTopicVecBinaryFile_vec); 
    
            Document doc;
            doc.add( *_CLNEW Field("sentence_gb",  senGB.c_str() , Field::STORE_NO | Field::INDEX_TOKENIZED) );
            doc.add( *_CLNEW Field("sentence_rawdata_offset", szSentenceRawDataOffset, Field::STORE_YES | Field::INDEX_NO ) );
            writer->addDocument( &doc );
            
            ++ ulSenCount;
    }

    fclose (outTopicVecBinaryFile[0]);
    fclose (outTopicVecBinaryFile[1]);
    fclose (outTopicVecBinaryFile[2]);
    fclose (outTopicVecBinaryFile[3]);
    //fclose (outTopicVecBinaryFile_vec);

    cout << "SenCount " <<ulSenCount << endl;
    // Make the index use as little files as possible, and optimize it
    
   
    writer->optimize();
    writer->close();
	_CLLDELETE(writer);
    
    cout << "Max sentence len" << maxLen << endl;	
    cout << "Finish & Close" << endl;	
	// printf("Indexing took: %d ms.\n\n", (int32_t)(Misc::currentTimeMillis() - str));
}


void IndexSentences(const char * path, const char * index )
{
    IndexWriter * writer = NULL;
    standard::StandardAnalyzer an;
	writer = _CLNEW IndexWriter(index, &an, true);
   
    int i = 0;
    //#define test 
    #ifdef test 
    ifstream infileGBSegFile ("./rawdata/segSenGB_afterSeg",std::ios::in|std::ios::binary);
    ifstream infileUTF8File ("./rawdata/segSenUTF8",std::ios::in|std::ios::binary);
    ifstream infileTopicVecFile ("./rawdata/topicsMTest",std::ios::in|std::ios::binary);
    ifstream infileClusterFile ("./rawdata/skResultTest",std::ios::in|std::ios::binary);
    #else
    //test remove stop word speed
    ifstream infileGBSegFile ("/home/david/David/corpus2.0/sentencesLib/ForLuceneIndex_WithOutStopWords",std::ios::in|std::ios::binary);
    //ifstream infileGBSegFile ("./finaldata/SensFilteredInvalidGB_Seg_Fileter11InvalidLDASens.dat",std::ios::in|std::ios::binary);
    ifstream infileUTF8File ("./finaldata/SensFilteredInvalidUTF8_Fileter11InvalidLDASens.dat",std::ios::in|std::ios::binary);
    ifstream infileTopicVecFile ("/home/david/David/lda++/GibbsLDA++-0.2/models/sentencesFinal/model-final.theta",std::ios::in|std::ios::binary);
    //ifstream infileGBSegFile ("./finaldata/SensFilteredInvalidGB_Seg.dat",std::ios::in|std::ios::binary);
    //ifstream infileUTF8File ("./finaldata/SensFilteredInvalidUTF8.dat",std::ios::in|std::ios::binary);
    //ifstream infileTopicVecFile ("./finaldata/topicsMatrix.txt_simple",std::ios::in|std::ios::binary);
    ifstream infileClusterFile ("./finaldata/beforeRemoveLDAInvalidSens/kMeansResult_20.txt",std::ios::in|std::ios::binary); // damy
   
    //SentenceInRawData * sentencesRawData = new SentenceInRawData [2565709L];

    #endif

    // To bypass a possible exception (we have no idea what we will be indexing...)
    writer->setMaxFieldLength(0x7FFFFFFFL); // LUCENE_INT32_MAX_SHOULDBE
    
    // Turn this off to make indexing faster; we'll turn it on later before optimizing
    //writer->setUseCompoundFile(false);
    writer->setUseCompoundFile(true);
    //writer->setMaxBufferedDocs(100);
    //writer->setMergeFactor(100);

    string senGB;
    string senUTF8;
    string topicVec;
    string cluster;
    
    unsigned long ulSenCount = 0;
    char szTopicOffset [32] = { 0 };
    char szSentenceRawDataOffset [32] = { 0 };
    int iStart = 0;
    int iEnd = 0;
    float fTopicVec[SENTENCE_LDA_TOPIC_DIM]; 
    
    FILE * outTopicVecBinaryFile;
    outTopicVecBinaryFile = fopen ("./sentencesCorpus.bin", "wb");

    SentenceInRawData sentenceInRawData;
    while( getline(infileGBSegFile, senGB,'\n') && \
           getline(infileUTF8File, senUTF8,'\n') && \
           getline(infileTopicVecFile, topicVec,'\n') && \
           getline(infileClusterFile, cluster,'\n')  ) {
            
            //cout << senGB <<endl;
            // cout<<" line" <<endl;

            
            if (ulSenCount % 10000 == 0){
                cout << "Sentence:" << ulSenCount <<endl;
                //writer->optimize();
            }
            //if (lSenCount > 100000)
            //cout << "Sentence:" << lSenCount <<" [" <<  senGB.c_str() << "]"  <<endl;
            Document doc;
            
            doc.add( *_CLNEW Field("sentence_gb",  senGB.c_str() , Field::STORE_NO | Field::INDEX_TOKENIZED) );
            doc.add( *_CLNEW Field("sentence_utf", senUTF8.c_str(), Field::STORE_YES | Field::INDEX_NO  ) );
            doc.add( *_CLNEW Field("topic_vec",    topicVec.c_str(), Field::STORE_YES | Field::INDEX_NO ) );
           
            // Store topic vec into another binary file 
            memset (szTopicOffset, 0, sizeof (szTopicOffset) );
            sprintf (szTopicOffset, "%lu", ulSenCount * SENTENCE_LDA_TOPIC_DIM * sizeof (float) ); 
            
            memset (&sentenceInRawData, 0, sizeof (SentenceInRawData));
            sprintf (szSentenceRawDataOffset, "%lu", ulSenCount * sizeof (SentenceInRawData) ); 
            strcpy (sentenceInRawData.szSentence, senUTF8.c_str ());
            sentenceInRawData.clusterID = 1;
           
            iStart = 0;

            for (int j=0; j< SENTENCE_LDA_TOPIC_DIM; j++){

                iEnd = topicVec.find (" ", iStart);
                float tmp = atof ( topicVec.substr (iStart, iEnd - iStart).c_str () );
                fTopicVec[j] = tmp; //atof ( topicVec.substr (iStart, iEnd - iStart).c_str () );
                sentenceInRawData.fTopicVec[j] = tmp;
                iStart = iEnd + 1;

            }

            //cout << endl;
            // fwrite(pVocabInfo, sizeof(VocabInfo),1,fout);

            //fwrite (fTopicVec, sizeof (fTopicVec), 1, outTopicVecBinaryFile); 
            fwrite (&sentenceInRawData, sizeof (SentenceInRawData), 1, outTopicVecBinaryFile); 
            
            doc.add( *_CLNEW Field("topic_vec_offset", szTopicOffset, Field::STORE_YES | Field::INDEX_NO ) );
            doc.add( *_CLNEW Field("sentence_rawdata_offset", szSentenceRawDataOffset, Field::STORE_YES | Field::INDEX_NO ) );
            //doc.add( *_CLNEW Field("topic_vec",    topicVec.c_str(), Field::STORE_YES | Field::INDEX_UNTOKENIZED ) );
            doc.add( *_CLNEW Field("cluster_id",   cluster.c_str(), Field::STORE_YES | Field::INDEX_NO ) );
            writer->addDocument( &doc );
            
            ++ ulSenCount;
    
    }

    fclose (outTopicVecBinaryFile);

    cout << "SenCount " <<ulSenCount << endl;
    // Make the index use as little files as possible, and optimize it
    writer->optimize();
    cout << "Finish optimize" << endl;	
    // Close and clean up
    writer->close();
	_CLLDELETE(writer);
    cout << "Finish & Close" << endl;	
	// printf("Indexing took: %d ms.\n\n", (int32_t)(Misc::currentTimeMillis() - str));
}

*/
