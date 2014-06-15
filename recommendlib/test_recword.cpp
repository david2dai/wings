#include "recword.h"
#include <iostream>

using namespace std;

int main (void) {
    
    //RecWord recWord ("/home/david/David/lucene/cluceneLib/recsentenceLib/recword_info_0111.bin");
    RecWord recWord ("/usr/share/ibus-pinyin/resources/word2vec_dict");
    //recWords.transformVocFormat ("/home/david/David/word2vec/src/zuoWenVectorFilterWXM.bin",
    //                             "./recword_info.bin");
    vector<WordInfo> words;
    recWord.searchWords ("栩栩如生", words);
    //recWord.searchWords ("多杀", words);
    //recWord.searchWords ("妈妈", words);
    //recWord.searchWords ("你好", words);
    //recWord.searchWords ("老师", words);
    //recWord.searchWords ("科学家", words);
    //recWord.searchWords ("中国", words);
    //recWord.searchWords ("聪明", words);
    //recWord.searchWords ("苹果", words);
    //recWord.searchWords ("我们", words);
    //recWord.searchWords ("奔跑", words);
    //recWord.searchWords ("挨门挨户", words);
    //recWord.searchWords ("卖报", words);

    for (int i=0;i<words.size ();++i) {
      cout << words[i].word << "\t" << words[i].sim << endl;
    } 
}
