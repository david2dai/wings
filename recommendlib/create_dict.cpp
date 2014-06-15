#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>


#include "dictionary.h"
using namespace std; 

int wordCmp (const void* a, const void* b){
    VocWordInfo* pa = (VocWordInfo*) a;
    VocWordInfo* pb = (VocWordInfo*) b;
    return (strcmp( pa->word, pb->word) );
}

void loadPOSDict (const char* posFile,
                  map<string, int>& podDict) {

    ifstream infilePOS (posFile); //, std::ios::in|std::ios::binary);
    string line;
    string word;
    string pos;
    int iStart = 0;
    int iEnd = 0;

    long count = 0;

    int ret = 0; // = Dictionary::posMapping (pos.c_str ());
    
    while( getline(infilePOS, line, '\n') ) {
       
        ++ count; 
        iEnd = line.find (" ");
        word = line.substr (iStart, iEnd-iStart);
        pos = line.substr (iEnd+1);
        
        ret = Dictionary::posMapping (pos.c_str ());
        podDict[word] = ret;
    }

    infilePOS.close ();
}

// not need export 
int transformVocFormat(const char* srcFile,
                       const char* posFile, 
                       const char* destFile) {
    FILE *f;
    long long a, b, c, d, cn;   // bi[100];
    char ch;
    float len;
    char szTmpWord[MAX_WORD_LEN] = {0};
    float fTmpVec[VOC_VEC_DIM] = {0};
    long long words = 0;
    long long size  = 0;

    map<string, int> posDict;
    loadPOSDict (posFile, posDict);

    VocWordInfo* pVocabInfo = new VocWordInfo[VOC_WORD_CNT];//(VocWordInfo*) malloc (sizeof (VocabInfo));
    memset(pVocabInfo,0x00,sizeof(VocWordInfo)*VOC_WORD_CNT);

    clock_t start, finish;
    start = clock();

    f = fopen(srcFile, "rb");
    if (f == NULL) {
        printf("Input file not found\n");
        return -1;
    }

    fscanf(f, "%lld", &words); //word count
    fscanf(f, "%lld", &size);  //vec dim

    printf("Voc info:words=%lld,size=%lld",words,size);

    //vocab = (char *)malloc((long long)words * MAX_WORD_LEN * sizeof(char));
    //M = (float *)malloc((long long)words * (long long)size * sizeof(float));

    //need resave a new file to speed the load time
    int maxWordLen = 0;
    int pos = 0;
    for (b = 0; b < words; b++) {
        fscanf(f, "%s%c", szTmpWord, &ch);
        strcpy(pVocabInfo[b].word, szTmpWord);

        if ( !strcmp ("栩栩如生", pVocabInfo[b].word)) {

            cout << "栩栩如生" << endl;
        }

        pos = posDict[szTmpWord];
        if (pos == 0) {
            cout << "error" << szTmpWord << endl;
        }

        pVocabInfo[b].pos = pos;

        int iLen = strlen(pVocabInfo[b].word);
        if( iLen > maxWordLen){
            maxWordLen = iLen;
            printf("[%s]\n",pVocabInfo[b].word);
        }

        for (a = 0; a < size; a++) fread(&fTmpVec[a], sizeof(float), 1, f);
        len = 0;
    
        for (a = 0; a < size; a++) len += fTmpVec[a] * fTmpVec[a];
        len = sqrt(len);

        for (a = 0; a < size; a++) {
            fTmpVec[a] /= len;
            pVocabInfo[b].vec[a] = fTmpVec[a];
        }
    }
    
    printf("Max word len is %d\n",maxWordLen);
    fclose(f);
    //Finish to load orginal data
    
    //Sort the voc
    qsort(pVocabInfo,VOC_WORD_CNT,sizeof(VocWordInfo),wordCmp);

    /*
    for (int i=0;i<words;++i) {
        if ( !strcmp (pVocabInfo[i].word, "栩栩如生") ) {
            cout << "栩栩如生 wordid" << i << endl;
        }
    }*/ 

    FILE * fout = fopen(destFile, "wb");
    fwrite(pVocabInfo, sizeof(VocWordInfo),VOC_WORD_CNT,fout);
    fclose(fout);

    delete [] pVocabInfo;
    pVocabInfo = NULL;
    finish = clock();

    double time = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("load vocab use time %f\n",time);
    return 0;
}

int main (void) {

    /*
    map<string, int> posDict;
    loadPOSDict ("/home/david/David/word2vec/wordCorpus/segCorpusWithPos_3/sortedDictWithMainPOS", posDict);

    
    int pos = posDict["黄如金"];
    
    cout << "Dict size is " << posDict.size () << endl;    
    if (pos & POS_NR) {
        cout << "OK!" << endl;
    } else {
        cout << "Fail!" << endl;
    }*/ 

    transformVocFormat("/home/david/David/word2vec/src/zuoWenVectorPOS2_0111.bin",
                       "/home/david/David/word2vec/wordCorpus/segCorpusWithPos_3/sortedDictWithMainPOS",
                       "./recword_info_0111.bin");
 
}
