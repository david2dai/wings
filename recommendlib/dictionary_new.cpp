#include "dictionary_new.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <math.h>

//using std::cout;
//using std::endl;

DictionaryNew::DictionaryNew (const char * szDictFolder) {
    
    m_pstIndex1 = new DictIndex1 [VOC_CH_HASH_SIZE];
    m_pstIndex2 = new DictWordInfoAndIndex2[VOC_WORD_CNT_NEW];
    m_pszWordsData = new char [VOC_WORD_SPACE_SIZE]; 
    
    char szFilePath [512] = {0};
    sprintf (szFilePath, "%s/%s", szDictFolder, "index1.bin");
    FILE * f_index1 = fopen (szFilePath, "rb");
    
    memset (szFilePath, 0, sizeof (szDictFolder));
    sprintf (szFilePath, "%s/%s", szDictFolder, "wordInfoAndIndex2.bin");
    FILE * f_index2 = fopen (szFilePath, "rb");    
    
    memset (szFilePath, 0, sizeof (szDictFolder));
    sprintf (szFilePath, "%s/%s", szDictFolder, "words.bin");
    FILE * f_words = fopen (szFilePath, "rb"); 

    fread (m_pstIndex1, sizeof (DictIndex1), VOC_CH_HASH_SIZE , f_index1);
    fread (m_pstIndex2, sizeof (DictWordInfoAndIndex2), VOC_WORD_CNT_NEW, f_index2);
    fread (m_pszWordsData, sizeof (char), VOC_WORD_SPACE_SIZE, f_words);

    fclose (f_index1);
    fclose (f_index2);
    fclose (f_words);
    
    cd_utf2gb = iconv_open ("gb2312//IGNORE", "utf-8//IGNORE");
    cd_gb2utf = iconv_open ("utf-8//IGNORE", "gb2312//IGNORE");
}

DictionaryNew::~DictionaryNew () {

    delete [] m_pstIndex1;
    m_pstIndex1 = NULL;
    delete [] m_pstIndex2;
    m_pstIndex2 = NULL;
    delete [] m_pszWordsData;
    m_pszWordsData = NULL;

    iconv_close(cd_utf2gb);
    iconv_close(cd_gb2utf);
}

DictionaryNew& DictionaryNew::GetInstance(const char* szDictFolder) {
    static DictionaryNew dict (szDictFolder);
    return dict;
}

const float * DictionaryNew::getWordVec (int wordId) {
    return m_pstIndex2 [wordId].vec;
}

int DictionaryNew::getWordPOS (int wordId) {
    return m_pstIndex2 [wordId].pos;
}

void DictionaryNew::getWord (int wordId, char * szUTF8Word, size_t size) {
    char * pWordStart = &m_pszWordsData [ m_pstIndex2 [wordId].offset ];
    memset (m_szTmpWordGB, 0, sizeof (m_szTmpWordGB));
    memset (szUTF8Word, 0, size);
    strncpy (m_szTmpWordGB, pWordStart, m_pstIndex2 [wordId].len);
    conGBToUTF8 (m_szTmpWordGB, szUTF8Word, size);
}

int DictionaryNew::getWordId (const char* orgUTF8Word) {
    int chHashId = 0;
 
    // convert UTF8 str to GB   
    memset (m_szTmpWordGB, 0, sizeof (m_szTmpWordGB));
    memset (m_szTmpWordUTF8, 0, sizeof (m_szTmpWordUTF8));
    strcpy (m_szTmpWordUTF8, orgUTF8Word);
    conUTF8ToGB (m_szTmpWordUTF8, m_szTmpWordGB, sizeof (m_szTmpWordGB));
   
    // search word in the dict 
    chHashId = CC_ID (m_szTmpWordGB [0], m_szTmpWordGB [1]);
    if (chHashId > VOC_CH_HASH_SIZE || 0==m_pstIndex1 [chHashId].cnt) {
        // Word not in Dict:
        // HashId out of range or The slot the m_pstIndex1[HashId] has no words. 
        return -1;
    } else {
        int low = m_pstIndex1 [chHashId].idx;
        int high = low + m_pstIndex1 [chHashId].cnt -1;
        int mid = 0;
        int offset = 0;
        int len = 0;
        int cmp = 0;
    
        while (low<=high) {
            mid = (low + high) / 2;
            offset = m_pstIndex2 [mid].offset;
            len = m_pstIndex2 [mid].len; 
            strncpy (m_szDictWordGB, m_pszWordsData + offset, len);
            m_szDictWordGB [len] = '\0';
            cmp = strcmp (m_szDictWordGB, m_szTmpWordGB);
            if (cmp == 0) {
                return mid;
            } else if (cmp < 0) {
                low = mid + 1;
            } else {
                high = mid - 1;
            } 
        } // while 
    }
    return -1; // word not in dict
}

float DictionaryNew::getPOSScore (int orgWordPos, int recWordPos) {
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

// used by segUTF8Str
void DictionaryNew::getAWord (char * start, int len, vector <string>& words) {
    memset (m_szBufferUTF8, 0, sizeof (m_szBufferUTF8));
    memset (m_szBufferGB, 0, sizeof (m_szBufferGB));
    strncpy (m_szBufferGB, start, len);
    conGBToUTF8 (m_szBufferGB, m_szBufferUTF8, sizeof (m_szBufferGB));
    words.push_back (m_szBufferUTF8); 
}

bool DictionaryNew::segUTF8Str (const char * szSentence, vector <string>& words) {
    memset (m_szBufferUTF8, 0, sizeof (m_szBufferUTF8));
    memset (m_szBufferGB, 0, sizeof (m_szBufferGB));
    strcpy (m_szBufferUTF8, szSentence);
    
    if ( conUTF8ToGB ( m_szBufferUTF8, m_szBufferGB, sizeof (m_szBufferGB)) ) {
        
        memset (m_szSentenceGB, 0, sizeof (m_szSentenceGB));
        strcpy (m_szSentenceGB, m_szBufferGB);
        unsigned char ch = 0;
        char* pCurCh = m_szSentenceGB;
        char* pWordStart = NULL;
        char szWord [128] = {0};
        
        int chHashId = 0;
        int segWordLen = 0;
        int startWordId=-1,wordCnt=0;
        int low=0,high=0,mid=0,cmpRet=0,len=0;
        long offset = 0;

        while ( (ch = *((unsigned char *) pCurCh)) != 0 ) {
            
            if(ch<176 || ch > 247)  // ch < 128;128 <= ch < 176;247 < ch
            {
                // Output a chinese word
                if (segWordLen) {
                    getAWord (pWordStart, segWordLen, words);
                    //strncpy (szWord, pWordStart , segWordLen);
                    //szWord [segWordLen] = 0;
                    //words.push_back (szWord); 
                    //cout <<  "1.Words = [" << szWord << "]" << endl;
                    segWordLen = 0;
                    pWordStart = NULL;
                }

                if (ch >= 128) {
                    ++pCurCh;
                }
                ++pCurCh;
            } else { // if(ch>=176 && ch<=247) Chinese character start
                
                if (0==segWordLen) { // New word start
                    pWordStart = pCurCh; 
                    chHashId = CC_ID (pWordStart[0],pWordStart[1]);
                   
                    if (chHashId > VOC_CH_HASH_SIZE || 0==m_pstIndex1 [chHashId].cnt) {
                    // Word not in Dict: HashId out of range or The slot the m_pstIndex1[HashId] has no words. 
                        // Output a chinese word
                        getAWord (pWordStart, segWordLen, words);
                        //strncpy (szWord, pWordStart, 2);
                        //szWord [segWordLen] = 0;
                        //words.push_back (szWord); 
                        //cout <<  "2.Words = [" << szWord << "]" << endl;
                        segWordLen = 0;
                        pWordStart = NULL;
                    } else {
                        startWordId = m_pstIndex1 [chHashId].idx; 
                        wordCnt = m_pstIndex1 [chHashId].cnt;
                        strncpy (szWord, pWordStart, 2);
                        segWordLen = 2;
                    }
                
                } else { // Continue the last word
                    low = startWordId;
                    high = low + wordCnt - 1;

                    while (low<=high) {
                        mid = (low + high) / 2;

                        offset = m_pstIndex2 [mid].offset;
                        len = m_pstIndex2 [mid].len;

                        if (len < segWordLen + 2) {
                            low = mid + 1;
                            //cout << "aaa" << endl;
                            continue;
                        }

                        cmpRet = strncmp (m_pszWordsData + offset + segWordLen, pCurCh, 2);
                        
                        // Find word in dict
                        if (!cmpRet) {
                            //search left
                            int minEqWordId = mid - 1;
                            while (minEqWordId >= low) {
                                offset = m_pstIndex2 [minEqWordId].offset;
                                len = m_pstIndex2 [minEqWordId].len;
                                if (len < segWordLen + 2) {
                                    break;
                                }

                                cmpRet = strncmp (m_pszWordsData + offset + segWordLen, pCurCh, 2);
                                if (cmpRet) {
                                    break;
                                }
                                --minEqWordId;  
                            }

                            //search right
                            int maxEqWordId = mid + 1;
                            while (maxEqWordId <= high) {
                                offset = m_pstIndex2 [maxEqWordId].offset;
                                len = m_pstIndex2 [maxEqWordId].len;

                                cmpRet = strncmp (m_pszWordsData + offset + segWordLen, pCurCh, 2);
                                if (cmpRet) {
                                    break;
                                }
                                ++maxEqWordId;
                            }

                            startWordId = minEqWordId + 1;
                            wordCnt = maxEqWordId - minEqWordId - 1;
                            //cout << "new range for next Chinese Ch startId=" << startWordId 
                            //   << " cnt=" << wordCnt << endl;
                            segWordLen += 2;
                            break;

                        } else if ( cmpRet < 0) {
                            low = mid + 1;
                        } else {
                            high = mid - 1;
                        }
                    }

                    // Word is not in dict, go on the next word
                    if (low > high) {
                        //strncpy (szWord, pWordStart, segWordLen);
                        getAWord (pWordStart, segWordLen, words);
                        //szWord [segWordLen] = 0;
                        //words.push_back (szWord); 
                        //cout <<  "3.Words = [" << szWord << "]" << endl;
                        segWordLen = 0;
                        pWordStart= NULL;

                        //Go on with the Current Chinese word (pCurch) in the next loop.
                        //without plusing 2 to pCurCh
                        continue;
                    }
                }
                pCurCh += 2;
            // 
            // A chinese ch finish
            } 
       
        // while End 
        // Seg a GB Sentence End 
        }

        if (segWordLen) {
            getAWord (pWordStart, segWordLen, words);
            //strncpy (szWord, pWordStart, segWordLen);
            //szWord [segWordLen] = 0;
            //cout <<  "4.Words = [" << szWord << "]" << endl;
            //words.push_back (szWord); 
            segWordLen = 0;
            pWordStart = NULL;
        }
        
        return true;
    // if conUTF8ToGB End
    }

    return false;
}

bool DictionaryNew::conUTF8ToGB ( char * utf8, char * gb, size_t gbsize )
{
    char *in = utf8;
    size_t inl = strlen (utf8);
    char *out = gb;
    size_t outl = gbsize;
    
    int ret = iconv (cd_utf2gb, &in, &inl, &out, &outl);
    if (ret < 0) {
        return false;
    }
    return true;
}

bool DictionaryNew::conGBToUTF8 ( char * gb, char * utf8, size_t utf8size )
{
    char *in = gb;
    size_t inl = strlen (gb);
    char *out = utf8;
    size_t outl = utf8size;
    
    int ret = iconv (cd_gb2utf, &in, &inl, &out, &outl);
    if (ret < 0) {
        return false;
    }
    return true;
}
