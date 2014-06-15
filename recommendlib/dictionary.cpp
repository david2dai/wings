#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <iostream>

#include "dictionary.h"

using std::cout;
using std::endl;

Dictionary::Dictionary (const char* fileName) {

    FILE* file_word2vec = fopen (fileName, "rb");

    //cout << "dict file " << fileName << endl;
    m_pVocWordsInfo = NULL;
    m_pVocWordsInfo = new VocWordInfo[VOC_WORD_CNT];
    
    memset (m_pVocWordsInfo, 0x00, sizeof(VocWordInfo)*VOC_WORD_CNT);
    fread (m_pVocWordsInfo, sizeof(VocWordInfo), VOC_WORD_CNT, file_word2vec);
    fclose (file_word2vec);
}

Dictionary::~Dictionary () {
    
    delete [] m_pVocWordsInfo;
    m_pVocWordsInfo = NULL;
}

Dictionary& Dictionary::GetInstance(const char* file_name) {

    static Dictionary dict (file_name);
    return dict;
}

const VocWordInfo& Dictionary::operator [] (long wordId) {

    return  m_pVocWordsInfo[wordId];

}

long Dictionary::getWordId (const char* orgWord) {

    // get word ids for orgWords
    long iMid = -1;
    long iLow = 0;
    long iHigh = VOC_WORD_CNT;

    while (iLow <= iHigh) {
        iMid = (iLow + iHigh) / 2;
        int iRet = strcmp (orgWord, m_pVocWordsInfo[iMid].word);

        // cout << "getWordId " << orgWord << iMid <<endl;
        if (iRet < 0) {
            iHigh = iMid -1;
        }else if (iRet == 0) {
            return iMid; //break;
        }else {
            iLow = iMid +1;
        }
    }

    return -1L;
}

int Dictionary::posMapping (const char * szPos) {

    int retPos = 0;

    if (strlen (szPos) == 2 && szPos[1] == 'l') {
        retPos |= POS_L;
    }

    switch (szPos[0]) {
        case 'a':
            retPos |= POS_A;
            break;
        case 'b':
            retPos |= POS_B;
            break;
        case 'c':
            retPos |= POS_C;
            break;
        case 'd':
            retPos |= POS_D;
            break;
        case 'e':
            retPos |= POS_E;
            break;
        case 'f':
            retPos |= POS_F;
            break;
        case 'k':
            retPos |= POS_K;
            break;
        case 'l': //cancel. use al vl nl
            retPos |= POS_L;
            break;
        case 'm':
            retPos |= POS_M;
            break;
        case 'n':
            {
                if (strlen (szPos) == 1) {
                    retPos |= POS_N;
                    break;
                }else {
                    switch (szPos[1]) {
                        case 'r':
                            retPos |= POS_NR;
                            break;
                        case 's':
                            retPos |= POS_NS;
                            break;
                        case 't':
                            retPos |= POS_NT;
                            break;
                        case 'z':
                            retPos |= POS_NZ;
                            break;
                        case 'l':
                            break;
                        default:
                            //cout << "Error" << endl;
                            break;
                    }
                }
            }
            break;
        case 'o':
            retPos |= POS_O;
            break;
        case 'p':
            retPos |= POS_P;
            break;
        case 'q':
            retPos |= POS_Q;
            break;
        case 'r':
            retPos |= POS_R;
            break;
        case 's':
            retPos |= POS_S;
            break;
        case 't':
            retPos |= POS_T;
            break;
        case 'u':
            retPos |= POS_U;
            break;
        case 'v':
            retPos |= POS_V;
            break;
        case 'y':
            retPos |= POS_Y;
            break;
        case 'z':
            retPos |= POS_Z;
            break;
        default:
            //cout << "pos mapping error" << endl;
            break;
    }
    
    return retPos;
}

float Dictionary::getPOSScore (int orgWordPos, int recWordPos) {

    float score = 1.0;
    //cout << "getPOSScore " << orgWordPos << " " << recWordPos << endl;
    
    if (recWordPos & POS_L) {
        score += 0.5;
        //cout << "rule 1: POS_L" << endl;
    }else { 
        if (orgWordPos & recWordPos) {
            score += 0.4;
            //cout << "rule 2: SAME POS" << endl;
        }else if (orgWordPos & POS_N && recWordPos & POS_A ) {
            score += 0.4;
            //cout << "rule 3: N + A" << endl;
        }else if (orgWordPos & POS_A && recWordPos & POS_N ) {
            score += 0.4;
            //cout << "rule 4: A + N" << endl;
        }else if (orgWordPos & POS_N && recWordPos & POS_V ) {
            score += 0.4;
            //cout << "rule 5: N + V" << endl;
        }else if (orgWordPos & POS_N && recWordPos & POS_Z ) {
            score += 0.4;
            //cout << "rule 6: N + Z" << endl;
        }else if (orgWordPos & POS_D && recWordPos & POS_V ) {
            score += 0.4;
            //cout << "rule 6: N + Z" << endl;
        }


    }

    return 1.0 + log (score);
}
