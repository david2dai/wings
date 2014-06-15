#include "ldainfer.h"
#include "timelog.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <vector>
#include <iostream>

using namespace std;

int main ( void ) {

    LDAInfer ldaInfer ("/home/david/David/lda++/GibbsLDA++-0.2/LDAModelWordInfo4Inf.bin", 
                       "/home/david/David/lucene/cluceneLib/LDAWordMapping.bin",
                        1.0, 0.1, 50);

    int MAX = 68787;// Total sentences
    
    srand(time(0));
    
    float topicVec[50];
    vector<long> words;

    TimeLog timeLog;
    char tmp[100] = {0};
    
    timeLog.start ();
    words.clear ();
    long word = 0;
    
    // 说明书 鸿雁 外语 六年 级 曲 桐 瑶 商品 名称 
    /*
    word = ldaInfer.ldaWordId ("说明书");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("鸿雁");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("外语");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("六年");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("级");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("曲");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("桐");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("瑶");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("商品");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("名称");
    words.push_back (word);
    */

    /*
    // 说 背叛 好听 骂 人
    word = ldaInfer.ldaWordId ("说");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("背叛");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("好听");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("骂");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("人");
    words.push_back (word);
    */

    // 班 难以 见到 说明 试验 成功
    word = ldaInfer.ldaWordId ("班");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("难以");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("见到");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("说明");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("试验");
    words.push_back (word);

    word = ldaInfer.ldaWordId ("成功");
    words.push_back (word);

    ldaInfer.inferSentence (words, topicVec);
    words.clear ();

    for (int i=0;i<50;++i) {
        cout << topicVec[i] << " ";
    }

    cout << endl;
    /* 
    for (int i=0;i<501;i++) {

        if (i>0 && i%100 == 0) {
            sprintf (tmp,"LDA infer %d used time",i);
            timeLog.end (tmp);
        }

        long wordId = m_ldaInfer->ldaWordId (szWord);
        words.clear ();
        for (int j=0;j<10;j++) {
            long word = rand() % MAX;
            words.push_back (word);
        }

    }*/

}
