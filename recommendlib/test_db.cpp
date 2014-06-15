#include <kcpolydb.h>
#include <stdlib.h>
#include <stdio.h>

#include "timelog.h"
#include "recsentence.h"


using namespace std;
using namespace kyotocabinet;


// main routine
int main(int argc, char** argv) {
    // create the database object
    PolyDB db;
    string str;

    // open the database


#define CREATE_
#ifdef CREATE 
    if (!db.open("../casket.kch", PolyDB::OWRITER | PolyDB::OCREATE)) {
    //if (!db.open("casket.kct", PolyDB::OWRITER | PolyDB::OCREATE)) {
        cerr << "open error: " << db.error().name() << endl;
    }

#else
    if (!db.open("../casket.kch", PolyDB::OREADER)) {
    //if (!db.open("casket.kct", PolyDB::OREADER)) {
        cerr << "open error: " << db.error().name() << endl;
    }
#endif

    // store records
    /*
       if (!db.set("foo", "hop") ||
       !db.set("bar", "step") ||
       !db.set("baz", "jump")) {
       cerr << "set error: " << db.error().name() << endl;
       }
       */

    TimeLog timeLog;

#ifdef CREATE 
    timeLog.start ();

    FILE * sen_file = fopen ("/home/david/David/lucene/cluceneLib/sentencesCorpus512.bin", "rb");
    SentenceInRawData * sentences = new SentenceInRawData[DOC_CNT];

    fread (sentences, sizeof (SentenceInRawData), DOC_CNT, sen_file);
    fclose (sen_file);

    for(long i=0;i<DOC_CNT;++i) {

        if(!db.set ((const char*) &i, sizeof (i),
                    (const char*) &sentences[i], sizeof (SentenceInRawData)) ) {
            cout << "error" << endl;
        }

    }

    timeLog.end ("insert data OK!");
    delete [] sentences;
    sentences = NULL;

#else
    cout << "Use the existed DB file" << endl;
#endif

    timeLog.start ();

    long MAX = DOC_CNT; // Total sentences
    srand(time(0));

    long key = 0L;
    size_t size = 0;

    for (int i=0;i<200;i++) {
        long docId = rand() % MAX;

        SentenceInRawData value;
        //int32_t get(const char* kbuf, size_t ksiz, char* vbuf, size_t max) = 0;
        if (-1 == db.get((const char *) &docId, sizeof (key), (char*) &value, sizeof (SentenceInRawData)) ) {
            cout << "Size error!" << endl;
            continue;
        }

        cout << "Key= " << docId << " value=" << value.szSentence << endl; 
    }

    timeLog.end ("get key value OK!");

    // close the database
    if (!db.close()) {
        cerr << "close error: " << db.error().name() << endl;
    }

    cout << "Finish!" << endl;
    return 0;
}
