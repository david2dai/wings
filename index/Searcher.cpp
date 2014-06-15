#define _ASCII
#include "CLucene.h"

#include <iostream>
#include <string>
#include <fstream>

#include <iconv.h>

using namespace std;

using namespace lucene::index;
using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::analysis;
using namespace lucene::queryParser;
using namespace lucene::search;


/*
int main(void) {
         char inbuf[] = "解决方法很简单";
         char outbuf[32];
         int ret;
         size_t inlen = strlen(inbuf);

         iconv_t cd = iconv_open( "UTF-8", "GBK" );
         char *in = inbuf;
         size_t inl = inlen;
         char *out = outbuf;
         size_t outl = 32;
         ret = iconv(cd, &in, &inl, &out, &outl);
         iconv_close(cd);
         if (ret < 0) {
             printf("iconv failed\n");
             return -1;
             }
         printf("iconv ok, outlen=%d\n", (int)outl);
         printf("%s\n", outbuf);

}
*/

void SearchFiles(const char* index){
        standard::StandardAnalyzer analyzer;
        //lucene::analysis::WhitespaceAnalyzer analyzer;

        char lineUTF[256] = { 0 };
        char lineGB[256] = { 0 };
        char * buf;

        iconv_t cd = iconv_open("gb2312//IGNORE", "utf-8//IGNORE");
        //iconv_t cd = iconv_open( "UTF-8", "GB2312" );
        
        IndexReader* reader = IndexReader::open(index);
        while (true) {
            printf("Enter query string: ");
            char* tmp = fgets(lineUTF,256,stdin);
            if ( tmp == NULL ) continue;
            
            int len = strlen(lineUTF);
            lineUTF[len-1]=0;
            
            
            char *in = lineUTF;
            size_t inl = len;
            char *out = lineGB;
            size_t outl = 256;
            int ret = iconv(cd, &in, &inl, &out, &outl);

            if (ret < 0) {
                printf("iconv failed\n");
                continue;
            }

            IndexSearcher s(reader);

            if ( len == 0 )
            break;

            Query* q = QueryParser::parse(lineGB,"sentence_gb",&analyzer);

            buf = q->toString();
            printf("Searching for: %s\n\n", buf);
            _CLDELETE_LCARRAY(buf);

            Hits* h = s.search(q);

            for ( size_t i=0;i<h->length() && i < 20;i++ ){
                Document* doc = &h->doc(i);
                /*
                printf("%d.%s,%s,%s - %f", i, doc->get(_T("sentence_utf")),\
                        doc->get(_T("topic_vec")),doc->get(_T("cluster_id")),h->score(i));
                */

                cout << i << "\t" << "[" << doc->get("sentence_utf") << "]\t" << h->score(i) << endl ;

            }

            _CLLDELETE(h);
            _CLLDELETE(q);

            s.close();
            
        }

        iconv_close(cd);

        reader->close();
        _CLLDELETE(reader);
}
