#include <iostream>

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stdint.h>
#include <string.h>

#define VOC_SIZE 68787 

using namespace std;

struct LDAWordMapping {
    char szWord[32];  // the max len of the word is 30 in the mapping file
    int wordId;
};

struct LDAWordMapping2 {
    char szWord[128];  // the max len of the word is 30 in the mapping file
    long wordId;
};
int cmp(const void * a, const void * b){
    LDAWordMapping* pa = (LDAWordMapping*) a;
    LDAWordMapping* pb = (LDAWordMapping*) b;
    return (strcmp( pa->szWord, pb->szWord) );
}

int main (void) {

    LDAWordMapping * m_pLDAWordMapping = new LDAWordMapping[VOC_SIZE];
    string ldaOrigWordsMappingFile ("/home/david/David/lda++/GibbsLDA++-0.2/models/sentencesFinal/wordmap.txt");  
    ifstream infile (ldaOrigWordsMappingFile.c_str (), std::ios::in|std::ios::binary);
  
    if ( !infile ) {
        //cout << "Open dict fial "<< ldaDictFile << endl;
        return 0;
    }

    string line;
    int iPos = 0;
    int MaxWordLen = 0;
    long wordCnt = 0;

    while (getline (infile, line, '\n')) {
        
        iPos = line.find (" ", 0);

        if (iPos == -1) {
            continue;
        }

        if (MaxWordLen < iPos) {
            MaxWordLen = iPos;
        }

        strcpy (m_pLDAWordMapping[wordCnt].szWord, line.substr (0,iPos).c_str());
        m_pLDAWordMapping[wordCnt].wordId =  atoi (line.substr (iPos+1).c_str());
        ++ wordCnt;
    }

    std::cout << "max word len " << MaxWordLen << std::endl; 
    qsort (m_pLDAWordMapping, VOC_SIZE, sizeof (LDAWordMapping), cmp);

    FILE * fout = fopen("./wordsMapping4LDA.bin", "wb");
    fwrite (m_pLDAWordMapping, sizeof (LDAWordMapping), VOC_SIZE, fout);
    fclose (fout);
    memset (m_pLDAWordMapping, 0, sizeof (LDAWordMapping) * VOC_SIZE);

    delete [] m_pLDAWordMapping;
    m_pLDAWordMapping = NULL;

}
