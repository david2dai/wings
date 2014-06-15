#include "ldainfer.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>

using namespace std;

int cmp(const void * a, const void * b){
    LDAWordMapping* pa = (LDAWordMapping*) a;
    LDAWordMapping* pb = (LDAWordMapping*) b;
    return (strcmp( pa->szWord, pb->szWord) );
}

LDAInfer::LDAInfer (const char * ldaModelWordInfoFile, 
                    const char* ldaDictFile, 
                    double dAlpha, double dBeta, int iterCnt )
    :alpha (dAlpha), beta (dBeta), m_iIterCnt (iterCnt), 
     m_piMatrixWordTopic (NULL), 
     m_piVecWordZ (NULL),
     m_pLDAModelWordInfo (NULL)
{
   
    //cout << "default alpha = " << alpha << " beta = " << beta << "sample iter = " << m_iIterCnt <<endl;
    m_pLDAModelWordInfo = new LDAModelWordInfo;
    m_pLDAWordMapping = new LDAWordMapping[VOC_SIZE];
    memset (m_pLDAWordMapping,0,sizeof (LDAWordMapping) * VOC_SIZE);

    /*    
    clock_t start, finish;
    double dtime = 0;
    start = clock();
    */

    FILE * inLDAModelBinaryFile;
    inLDAModelBinaryFile = fopen (ldaModelWordInfoFile, "rb");
    fread (m_pLDAModelWordInfo, sizeof (LDAModelWordInfo), 1, inLDAModelBinaryFile); 
    fclose (inLDAModelBinaryFile);

    /*
    finish = clock();
    dtime = (double)(finish - start) / CLOCKS_PER_SEC;
    cout << "LDA Load "<< ldaModelWordInfoFile <<" used time is :" << dtime << " s" << endl;

    start = clock();
    */

    //int WordCnt = 
    loadLDADict (ldaDictFile);
   
    /* 
    finish = clock();
    dtime = (double)(finish - start) / CLOCKS_PER_SEC;
    cout << "LDA Load "<< ldaDictFile<<" used time is :" << dtime << " s" << endl;
    */
}

LDAInfer::~LDAInfer ( ) 
{
    //cout << "LDA destrucotor Start!" << endl;
    delete m_pLDAModelWordInfo;
    m_pLDAModelWordInfo = NULL;

    delete [] m_pLDAWordMapping;
    m_pLDAWordMapping = NULL;

    if (m_piMatrixWordTopic) {
        for (int m = 0; m < m_iSentenceVocSize; m++) {
            if (m_piMatrixWordTopic[m]) {
                delete [] m_piMatrixWordTopic[m];
            }
        }
    }

    delete [] m_piMatrixWordTopic;
    delete [] m_piVecWordZ;
    //cout << "LDA destrucotor OK!" << endl;
}

long LDAInfer::loadLDADict (const char* ldaDictFile) 
{
    FILE * fWordMappingBin = fopen (ldaDictFile, "rb");
    fread (m_pLDAWordMapping, sizeof (LDAWordMapping), VOC_SIZE, fWordMappingBin);
    fclose (fWordMappingBin);
    return VOC_SIZE;
}

long LDAInfer::ldaWordId (const char* word) 
{
    LDAWordMapping * pRet = NULL;
    //sizeof(LDAWordMapping), VOC_SIZE
    pRet = (LDAWordMapping *) bsearch (word, m_pLDAWordMapping, VOC_SIZE, sizeof (LDAWordMapping), cmp);

    if (NULL == pRet){
        return -1;
    }
    
    return pRet->wordId;
}

bool LDAInfer::inferSentence (const Sentence& sentence, float* senTopicVec) 
{
    initInfer (sentence);
    infer (sentence);
    //inferSampling (sentence, int n);
    computeTheta (senTopicVec);
    return true;
}

int LDAInfer::initInfer (const Sentence& sentence) 
{
    // estimating the model from a previously estimated one
    // map
    m_SentenceDict.clear ();
    int sentenceWordId = 0;
    int sentenceSize =  sentence.size ();
    for (int i=0;i<sentenceSize;i++) {
        // add words to dict
        if (m_SentenceDict.find (sentence[i]) == m_SentenceDict.end () ) { // new word
                m_SentenceDict [sentence[i]] = sentenceWordId;
                // cout << " sentence word id mapping " << sentence[i] << " to " << sentenceWordId << endl ;
                ++ sentenceWordId ;
        }
    }

    // different words in sentence
    m_iSentenceVocSize = m_SentenceDict.size ();

    // cout << " sentence word voc size" << m_iSentenceVocSize << endl ;
   
    // word w assigned to topic t times
    m_piMatrixWordTopic = new int*[m_iSentenceVocSize];
    
    for (int w = 0; w < m_iSentenceVocSize; w++) {
        m_piMatrixWordTopic[w] = new int[TOPIC_DIM];
        
        for (int k = 0; k < TOPIC_DIM; k++) {
    	    m_piMatrixWordTopic[w][k] = 0;
        }
    }

    // because there is only one sentece to infer
    // so m_iVecDocTopic and m_iVecWordTopicSum are the same.
    // the words of doc assigned to topic t 
    for (int k = 0; k < TOPIC_DIM; k++) {
        m_iVecDocTopic[k] = 0;
    }

    // the total words assigned to topic z 
    for (int k = 0; k < TOPIC_DIM; k++) {
	    m_iVecWordTopicSum[k] = 0;
    }
   
    // sentece assiged by total topic
    // or total word of sentence 
    m_iDocTopicSum = 0;

    srandom(time(0)); // initialize for random number generation
   
    // store the result word:z for each word of the sentence 
    // I find this may be a bug
    // the size of new arry is not correct. 2014.03.06
    //m_piVecWordZ = new int[m_iSentenceVocSize]; // <--fix this bug
    m_piVecWordZ = new int[sentence.size ()];  //updated 2014.03.12

	// assign values for nw, nd, nwsum, and ndsum	
    for (int n = 0; n < sentenceSize; n++) {
        
        int _w = m_SentenceDict[sentence[n]];
        
        int topic = (int)(((double)random() / RAND_MAX) * TOPIC_DIM);
        m_piVecWordZ[n] = topic;

        // number of instances of word i assigned to topic j
        m_piMatrixWordTopic[_w][topic] += 1;
        // number of words in document i assigned to topic j
        m_iVecDocTopic[topic] += 1;
        // total number of words assigned to topic j
        m_iVecWordTopicSum[topic] += 1;
    } 
    // total number of words in document i
    m_iDocTopicSum = sentence.size ();      
     
    return 0;        
}

void LDAInfer::infer (const Sentence& sentence) 
{
    int sentenceSize = sentence.size (); 
    for (int iter = 1; iter <= m_iIterCnt; iter++) {
        	
        for (int n = 0; n < sentenceSize; n++) {
            // sample from p(z_i|z_-i, w)
            int topic = inferSampling(sentence, n);
            m_piVecWordZ[n] = topic;
        }
    }
   
    /*
    for (int n = 0; n < sentence.size (); n++) {
        cout << sentence[n] << ":" << m_piVecWordZ[n] << " ";
    }
    
    cout << endl;
    */
}

int LDAInfer::inferSampling(const Sentence& sentence, int n) 
{
    // remove z_i from the count variables
    int topic = m_piVecWordZ[n];
    long w = sentence[n];
    int _w = m_SentenceDict[sentence[n]];
    
    m_piMatrixWordTopic[_w][topic] -= 1;
    m_iVecDocTopic[topic] -= 1;
    m_iVecWordTopicSum[topic] -= 1;
    m_iDocTopicSum -= 1;
    
    float Vbeta = VOC_SIZE * beta;
    float Kalpha = TOPIC_DIM * alpha;
    // do multinomial sampling via cumulative method
    
    for (int k = 0; k < TOPIC_DIM; k++) {

        m_fProb[k] = (m_pLDAModelWordInfo->nw[w][k] + m_piMatrixWordTopic[_w][k] + beta) /\
                     (m_pLDAModelWordInfo->nwsum[k] + m_iVecWordTopicSum[k] + Vbeta) *\
		             (m_iVecDocTopic[k] + alpha) / (m_iDocTopicSum + Kalpha);

    }

    // cumulate multinomial parameters
    for (int k = 1; k < TOPIC_DIM; k++) {
        m_fProb[k] += m_fProb[k - 1];
    }
    
    // scaled sample because of unnormalized p[]
    double u = ((double)random() / RAND_MAX) * m_fProb[TOPIC_DIM - 1];
    
    for (topic = 0; topic < TOPIC_DIM; topic++) {
        if (m_fProb[topic] > u) {
            break;
        }
    }
    
    // add newly estimated z_i to count variables
    m_piMatrixWordTopic[_w][topic] += 1;
    m_iVecDocTopic[topic] += 1;
    m_iVecWordTopicSum[topic] += 1;
    m_iDocTopicSum += 1;    
   
    return topic;
}

void LDAInfer::computeTheta (float* senTopicVec) 
{
	for (int k = 0; k < TOPIC_DIM  ; k++) {
	    senTopicVec[k] = (m_iVecDocTopic[k] + alpha) / (m_iDocTopicSum + TOPIC_DIM * alpha);
	}
}
