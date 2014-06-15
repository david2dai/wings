//#include "dictionary.h"
#include "dictionary_new.h"

#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <memory.h>

#include <vector>


using namespace std;

bool conUTF8ToGB ( char * utf8, char * gb, size_t gbsize );

void createNewDict ();
void loadNewDict();

int main (void) {

    //createNewDict ();
    //DictionaryNew dictNew ("./word2vec_dict");
    DictionaryNew& dictNew = DictionaryNew::GetInstance ();
    vector <string> words;
    dictNew.segUTF8Str ("欢迎光临;哈尔滨工业大学。智能技术与自然语言处理实验室!", words);
    for (int i=0;i<words.size ();++i) {
        cout << words [i] << " ";
    }
    cout << endl;
    words.clear ();
    dictNew.segUTF8Str ("一只美丽可爱的小花猫", words);
    for (int i=0;i<words.size ();++i) {
        cout << words [i] << " ";
    }
    cout << endl;
    int wordId = dictNew.getWordId ("栩栩如生");
    cout << wordId << endl;
    cout << dictNew.getWordId ("牡丹") << endl;
    char tmpWord [64] = {0};
    dictNew.getWord (wordId, tmpWord, sizeof (tmpWord));
    cout << tmpWord << endl;
    dictNew.getWord (54840, tmpWord, sizeof (tmpWord));
    cout << tmpWord << endl;
    memset (tmpWord, 0, sizeof (tmpWord));
    dictNew.getWord (54843, tmpWord, sizeof (tmpWord));
    cout << tmpWord << endl;
    if (dictNew.getWordPOS (wordId) & POS_L) {
        cout << "POS is L" << endl;
    }
    
    words.clear ();
    //loadNewDict ();
}

long searchWord (const char * szWord, DictIndex1 * index1, 
        DictWordInfoAndIndex2 * index2, char * wordsData) {

    char wordUTF8[128] = {0};
    char wordGB[128] = {0};
    int idx1 = 0;
    int chHashId = 0;
    int low = 0;
    int mid = 0;
    int high = 0;
    int cmpret = 0;
    long offset = 0;
    
    strcpy (wordUTF8, szWord);

    char temp [128] = {0};
    int len = 0;
    int cnt = 0;

    if ( conUTF8ToGB ( wordUTF8, wordGB, sizeof (wordGB) ) ) {

        chHashId = CC_ID (wordGB[0],wordGB[1]);

        if (chHashId > VOC_CH_HASH_SIZE) {
            return -1;
        }

        idx1 = index1 [chHashId].idx; 
        cnt = index1 [chHashId].cnt;
        
        low = idx1;
        high = idx1 + cnt - 1;

        while (low <= high) {
            
            mid = (low + high) / 2;
            offset = index2 [mid].offset;
            len = index2 [mid].len;
            
            strncpy (temp, wordsData + offset, len);
            temp [len] = 0;

            cmpret = strcmp (wordGB,temp);
            if ( ! cmpret ) {
                std::cout << "Find "<< szWord << " id=" << offset << std::endl;
                return offset;
            } else if ( cmpret < 0) {
                high = mid-1;
            } else {
                low = mid+1;
            }
        }

        if (low > high) {
            std::cout << "Not find " << szWord << std::endl;
        }

    }

    return -1;
}

void loadNewDict() {

    DictIndex1 * index1 = new DictIndex1 [VOC_CH_HASH_SIZE];
    DictWordInfoAndIndex2 * index2 = new DictWordInfoAndIndex2[VOC_WORD_CNT_NEW];
    char * wordsData = new char [VOC_WORD_SPACE_SIZE]; 

    FILE * f_index1 = fopen ("./word2vec_dict/index1.bin", "rb");    
    FILE * f_index2 = fopen ("./word2vec_dict/wordInfoAndIndex2.bin", "rb");    
    FILE * f_words = fopen ("./word2vec_dict/words.bin", "rb"); 

    fread (index1, sizeof (DictIndex1), VOC_CH_HASH_SIZE , f_index1);
    fread (index2, sizeof (DictWordInfoAndIndex2), VOC_WORD_CNT_NEW, f_index2);
    fread (wordsData, sizeof (char), VOC_WORD_SPACE_SIZE, f_words);

    fclose (f_index1);
    fclose (f_index2);
    fclose (f_words);

    char temp [128] = {0};
    long off = 0;
    int len = 0;
    int idx1 = 0;
    int cnt = 0;

    for (int j=0;j<VOC_CH_HASH_SIZE;j++) {
        
        //std::cout << "idx=" << index1 [j].idx << " cnt=" << index1 [j].cnt << std::endl;
        idx1 = index1 [j].idx;
        cnt = index1 [j].cnt;

        for (int k=idx1;k<idx1+cnt;++k) {
            off = index2 [k].offset;
            len = index2 [k].len;
            
            strncpy (temp, wordsData + off, len);
            temp [len] = 0;
            //std::cout << "ch words is " << temp << std::endl;

        }
    }

    searchWord ("栩栩如生1", index1, index2, wordsData);
    searchWord ("栩栩如生", index1, index2, wordsData);
    
    delete [] index1;
    index1 = NULL;
    delete [] index2;
    index2 = NULL;
    delete [] wordsData;
    wordsData = NULL;
}

// the following is the create new dict from the old
/*
int cmp ( const VocWordInfo& a, const VocWordInfo & b )
{

    if ( strcmp (a.word, b.word) < 0) {
        return 1;
    }
    else {
        return 0;
    }
}

void createNewDict () {

    DictIndex1 * index1 = new DictIndex1 [VOC_CH_HASH_SIZE];
    DictWordInfoAndIndex2 * index2 = new DictWordInfoAndIndex2[VOC_WORD_CNT_NEW];
    char * wordsData = new char [VOC_WORD_SPACE_SIZE]; 
     
    Dictionary & dict = Dictionary::GetInstance ();
    char wordUTF8[128] = { 0 };
    char wordGB[128] = { 0 };
    unsigned char * p;
    bool isChWord = true;
    int minId = 10000;
    int maxId = 0;
    int chId = 0;
    int firstChCnt = 0;
    int firstChSubCnt = 0;
    int firstChSubCntSum = 0;

    long offset = 0;
    int wordlen = 0;

    int wordId = 0;
    char lastFirstCh [3] = {0};


    vector <VocWordInfo> * sortDict = new  vector <VocWordInfo>(VOC_WORD_CNT);
    vector <VocWordInfo> & sortDict2 = * sortDict;
    // Need change utf8 to gb and then sort
    for (int i=0;i<VOC_WORD_CNT;i++) {

        memset (wordUTF8, 0, sizeof (wordUTF8));
        memset (wordGB, 0, sizeof (wordGB));
        
        strcpy (wordUTF8, dict[i].word);
        memset (sortDict2[i].word, 0, sizeof (sortDict2[i].word));
        
        if ( conUTF8ToGB ( wordUTF8, wordGB, sizeof (wordGB) ) ) {
            memcpy (&sortDict2[i], &dict[i], sizeof (VocWordInfo));
            strcpy (sortDict2[i].word, wordGB);
        }
    }

    sort (sortDict2.begin (), sortDict2.end (), cmp); 

    std::cout << "start" << std::endl;
    for (int i=0;i<VOC_WORD_CNT;i++) {
        isChWord = true;
        
        //strcpy (wordUTF8, sortDict2[i].word);
        //std::cout << wordUTF8 << std::endl;
        //sprintf (wordUTF8, "%s", dict[i].word);
        //if ( conUTF8ToGB ( wordUTF8, wordGB, sizeof (wordGB) ) ) {
            p = (unsigned char *) sortDict2[i].word; //wordGB;
            //std::cout << wordGB << std::endl;
            if (strlen (sortDict2[i].word) == 0 ) {
                isChWord = false;
            }

            while ( *p != '\0' ) {
                if(*p<128) {
                    isChWord = false;
                    break;
                } else if(*p==162) {
                    isChWord = false;
                    break;
                } else if(*p==163&&*(p+1)>175&&*(p+1)<186) {
                    isChWord = false;
                    break;
                } else if(*p==163&&(*(p+1)>=193&&*(p+1)<=218||*(p+1)>=225&&*(p+1)<=250)) {
                    isChWord = false;
                    break;
                } else if(*p==161||*p==163) {
                    isChWord = false;
                    break;
                } else if(*p>=176&&*p<=247) {
                    p += 2;
                    //std::cout << "chinese word" << wordGB << std::endl;
                } else {
                    isChWord = false;
                    break;
                }

            }

            if (isChWord) { 
                //memset (wordUTF8, 0, sizeof (wordUTF8));
                //conUTF8ToGB ( wordGB, wordUTF8, sizeof (wordUTF8) );
                
                strcpy (wordsData + offset, sortDict2[i].word); 
                // cpy data from old dict 
                memcpy (index2 [wordId].vec, sortDict2[i].vec, VOC_VEC_DIM * sizeof (float));
                index2 [wordId].pos = sortDict2[i].pos;
                index2 [wordId].offset = offset;
                wordlen = strlen (sortDict2[i].word);
                index2 [wordId].len= wordlen;
                offset += wordlen;
                ++wordId;
                
                cout << "222 i=" <<i << " /" << VOC_WORD_CNT << endl;
                //strcpy (wordUTF8, dict[i].word);

                if (strncmp (lastFirstCh, sortDict2[i].word,2)) {
                    if (firstChSubCnt != 0) {
                        std::cout << lastFirstCh << "\t" << firstChSubCnt  << std::endl;
                        chId = CC_ID (lastFirstCh[0],lastFirstCh[1]);
                        index1 [chId].idx = firstChSubCntSum;
                        index1 [chId].cnt = firstChSubCnt;
                        std::cout <<  "idx=" << firstChSubCntSum << "\t cnt" << firstChSubCnt<< std::endl;

                    }
                    ++firstChCnt;
                    lastFirstCh [0]= sortDict2[i].word[0];
                    lastFirstCh [1]= sortDict2[i].word[1];
                    // fisrt chinese word in all words
                    firstChSubCntSum += firstChSubCnt; 
                    firstChSubCnt = 1;
                } else {

                    ++firstChSubCnt;
                }

                chId = CC_ID (sortDict2[i].word[0],sortDict2[i].word[1]);
                if (chId > maxId) {
                    maxId = chId;
                }
                if (chId < minId) {
                    minId = chId;
                }
                //std::cout << wordGB << std::endl;
                //std::cout << wordUTF8 << std::endl;
            }

        //}
    }

    //---------------------------------------------------------------------------------
    // the last word
    std::cout << lastFirstCh << "\t" << firstChSubCnt  << std::endl;
    std::cout << "7777777777777777777" << std::endl;
    chId = CC_ID (lastFirstCh[0],lastFirstCh[1]);
    index1 [chId].idx = firstChSubCntSum;
    index1 [chId].cnt = firstChSubCnt;
    
    std::cout << "8888888888888888888888" << std::endl;
    
    std::cout <<  "idx=" << firstChSubCntSum << "\t cnt" << firstChSubCnt<< std::endl;
    firstChSubCntSum += firstChSubCnt; 
    //---------------------------------------------------------------------------------

    std::cout << "=============================" << std::endl;
    std::cout << "All words " << wordsData << std::endl;
    char temp [128] = {0};
    for (int j=0;j<VOC_CH_HASH_SIZE;j++) {
        
        std::cout << "idx=" << index1 [j].idx << " cnt=" << index1 [j].cnt << std::endl;
        int idx1 = index1 [j].idx;
        int cnt = index1 [j].cnt;

        for (int k=idx1;k<idx1+cnt;++k) {
            long off = index2 [k].offset;
            int len = index2 [k].len;
            //memset (temp, 0, sizeof (temp));
            strncpy (temp, wordsData + off, len);
            temp [len] = 0;
            std::cout << "ch words is " << temp << std::endl;


        }
    }


    FILE * f_index1 = fopen ("./word2vec_dict/index1.bin", "wb");    
    FILE * f_index2 = fopen ("./word2vec_dict/wordInfoAndIndex2.bin", "wb");    
    FILE * f_words = fopen ("./word2vec_dict/words.bin", "wb"); 

    fwrite (index1, sizeof (DictIndex1), VOC_CH_HASH_SIZE , f_index1);
    fwrite (index2, sizeof (DictWordInfoAndIndex2), VOC_WORD_CNT_NEW, f_index2);
    fwrite (wordsData, sizeof (char), VOC_WORD_SPACE_SIZE, f_words);

    fclose (f_index1);
    fclose (f_index2);
    fclose (f_words);

    delete [] index1;
    index1 = NULL;
    delete [] index2;
    index2 = NULL;
    delete [] wordsData;
    wordsData = NULL;
    std::cout << "max=" << maxId << " min=" << minId << " count=" << firstChCnt 
        <<" total words=" << firstChSubCntSum << std::endl;
    std::cout << "offset =" << offset << " end" << std::endl;
}

*/

bool conUTF8ToGB ( char * utf8, char * gb, size_t gbsize )
{
    char *in = utf8;
    size_t inl = strlen (utf8);
    char *out = gb;
    size_t outl = gbsize;

    iconv_t cd;
    
    cd = iconv_open ("gb2312//IGNORE", "utf-8//IGNORE");
    int ret = iconv (cd, &in, &inl, &out, &outl);
    
    iconv_close(cd);


    if (ret < 0) {
        
        return false;
    }

    return true;
}
/*
int charType(unsigned char *sChar)
{
  if(*sChar<128)
  {
	 if(strchr("\042!,.?()[]{}+=",(int)*sChar))
		 return CT_DELIMITER;
	 return CT_SINGLE;
  }
  else if(*sChar==162)
	  return CT_INDEX;
  else if(*sChar==163&&*(sChar+1)>175&&*(sChar+1)<186)
	  return CT_NUM;
  else if(*sChar==163&&(*(sChar+1)>=193&&*(sChar+1)<=218||*(sChar+1)>=225&&*(sChar+1)<=250))
	  return CT_LETTER;
  else if(*sChar==161||*sChar==163)
	  return CT_DELIMITER;
  else if(*sChar>=176&&*sChar<=247)
      return CT_CHINESE;
  else
      return CT_OTHER;
}

*/

