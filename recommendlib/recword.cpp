#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <memory.h>
#include <queue>
#include <iostream>
#include <algorithm>

#include "recword.h"

//using namespace std;
using std::priority_queue;
using std::cout;
using std::endl;

struct WordIdAndSim {

    long id;
    float sim;
};

struct WordIdAndSimCmp {

    bool operator() (WordIdAndSim& a,WordIdAndSim& b) 
    {   
        return a.sim > b.sim;
    }   
};

struct WordInfoCmpWithPos {

    bool operator() (const WordInfo& a,const WordInfo& b) 
    {   
        return a.sim*a.posScore > b.sim*b.posScore;
    }   
};

const unsigned int RecWord::REC_WORD_COUNT=200;

RecWord::RecWord (const char * szDictFolder)
    //:m_dict (Dictionary::GetInstance (file_name)) {
    :m_dict (DictionaryNew::GetInstance (szDictFolder)) {

}

RecWord::~RecWord () {
  
}

int RecWord::normalize (float* fVec, int dim) {
    float len = 0.0; 
    for (int i=0;i<dim;++i) len += fVec[i]*fVec[i];

    if (len<1.0e-6) {
       return -1;
    }
   
    len = sqrt (len); 
    for (int i=0;i<dim;++i) fVec[i] /= len;
    return 0; 
}

void RecWord::addWord2LocalContext (const char* szWord) {

    //long wordId = m_dict.getWordId (szWord);
    int wordId = m_dict.getWordId (szWord);

    if (-1==wordId) {
        return;
    }

    m_localContextWordIdsVec.push_back (wordId);
    while (m_localContextWordIdsVec.size () > 2) {
        m_localContextWordIdsVec.erase (m_localContextWordIdsVec.begin ());
    }
}

long RecWord::searchWords (const char* origWord, vector<WordInfo>& retWords) {
    
    //long origWordId = m_dict.getWordId (origWord);
    //long localContextWordId = 0;
    int origWordId = m_dict.getWordId (origWord);
    int localContextWordId = 0;
    
    if ( (-1==origWordId)) { // && 0==m_localContextWordIdsVec.size () ) {
        return -1;
    }

    // local context
    for (int i=0;i<VOC_VEC_DIM;++i) m_fLocalContextVec[i] = 0.0;
    const float * pfVec = NULL;
    for (unsigned int i=0;i<m_localContextWordIdsVec.size ();++i) {
        localContextWordId = m_localContextWordIdsVec[i];
        pfVec =  m_dict.getWordVec (localContextWordId);
        for (int j=0;j<VOC_VEC_DIM;++j) {
            m_fLocalContextVec[i] += pfVec[j];
        }
    }
    
    //addWord2LocalContext (origWordId);
    const float * pfOrigWordVec = m_dict.getWordVec (origWordId);
    memcpy (m_fOrigWordVec, pfOrigWordVec, VOC_VEC_DIM * sizeof (float));
    //for (int i=0;i<VOC_VEC_DIM;++i) m_fOrigWordVec[i] = pfOrigWordVec[i];

    normalize (m_fLocalContextVec, VOC_VEC_DIM);

    int origWordPos = m_dict.getWordPOS (origWordId);
    for (int i=0;i<VOC_VEC_DIM;++i) {
        m_fOrigWordAndContextVec[i] = 0 * m_fLocalContextVec[i] + 1.0 * m_fOrigWordVec[i];
    }

    normalize (m_fOrigWordAndContextVec, VOC_VEC_DIM);
    // priority queue to get max 100 words
    priority_queue <WordIdAndSim, vector<WordIdAndSim>, WordIdAndSimCmp> pq;
    float sim = 0.0;
    long id = -1;

    for (int i=0;i<VOC_WORD_CNT_NEW;++i) {

        if (i==origWordId) {
           continue;
        }
        sim = 0.0;
        pfVec = m_dict.getWordVec (i);
        for (int j=0;j<VOC_VEC_DIM;++j) {
            sim += pfVec[j] * m_fOrigWordAndContextVec[j];
        }
       
        id = i;
        // Size
        if (pq.size () < REC_WORD_COUNT) {
           
            WordIdAndSim wordIdAndSim = {id, sim};
            pq.push (wordIdAndSim);
        
        }else {
            if (pq.top ().sim < sim) {

                while (pq.size ()>= REC_WORD_COUNT) {
                    pq.pop ();
                }
                WordIdAndSim wordIdAndSim = { id, sim };
                pq.push (wordIdAndSim);
            }
        }
    }
  
    while (pq.size () > 0) {
        WordIdAndSim top = pq.top ();
        WordInfo wordInfo = {"", top.sim, m_dict.getWordPOS (top.id), 0.0, top.id, 0};
        m_dict.getWord (top.id, wordInfo.word, sizeof (wordInfo.word));
        //strcpy (wordInfo.word, m_dict[top.id].word);
        retWords.push_back (wordInfo);
        pq.pop ();
    }

    float score = 0.0;
    int retWordsSize = retWords.size ();
    for (int i=0;i<retWordsSize;++i) {
        score = m_dict.getPOSScore (origWordPos, retWords[i].pos);
        if (strlen (retWords[i].word)== 3) { //one chinese word
            score *= 0.9; // discount
        }
        retWords[i].posScore = score; 
    }

    // The words will be sorted in the PYPhoneticEditor.cc 
    // It will use sim/POS/feedback to re-rank
    //sort (retWords.begin (), retWords.end (), WordInfoCmpWithPos());

    return origWordId;
}
