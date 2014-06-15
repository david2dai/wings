CC = gcc 
CXX = g++
CFLAGS  = -fPIC -Wall -O3 -Wno-unused-result
CPPFLAGS = 
CXXFLAGS = -fPIC -Wall -O3 -Wno-unused-result
LDFLAGS  = -shared 

OBJECTS = timelog.o ldainfer.o recsentence.o cachefile.o \
		  recword.o dictionary_new.o log.o

DESTDIR = /usr

INCLUDE_HEADER_LUCENE = -I/usr/include/CLucene
LIB_LUCENE = -lclucene-core  -lm

all: librecommend.so

librecommend.so: ${OBJECTS}
	${CXX}  ${LDFLAGS} -o $@     \
		${INCLUDE_HEADER_LUCENE} \
		${OBJECTS} \
		${LIB_LUCENE}

# auto add the .cpp to the .o's dependance
# use the default rule of make: CXX CXXFLAGS -c -o xx.o xx.cpp
timelog.o: timelog.h
ldainfer.o: ldainfer.h
cachefile.o: cachefile.h
recsentence.o: recsentence.h ldainfer.h timelog.h cachefile.h
recword.o: recword.h dictionary_new.h
dictionary_new.o: dictionary_new.h
log.o: log.h

# old dictionary 
#recword.o: recword.h dictionary.h 
#dictionary.o: dictionary.h
#chseg.o: chseg.h dictionary.h

# $< is x.cpp   $* is x
#%.o: %.cpp %.h
#	g++ $(CFLAGS)  $< -o $*.o

old: timelog.h timelog.cpp ldainfer.h ldainfer.cpp \
	recsentence.h recsentence.cpp cachefile.h cachefile.cpp \
	recword.h recword.cpp dictionary.h  dictionary.cpp chseg.h chseg.cpp
	 g++ -o  librecommend.so \
		 -shared -fPIC \
		 -I/usr/include/ -I/usr/include/CLucene  \
		 timelog.cpp \
		 recsentence.cpp \
		 cachefile.cpp  \
		 ldainfer.cpp \
		 dictionary.cpp \
		 recword.cpp \
		 chseg.cpp \
		 log.cpp  \
		 -lclucene-core 

#-lpthread

install: librecommend.so 
	 -mkdir -p ${DESTDIR}/include/recommend
	 -mkdir -p ${DESTDIR}/lib
	 cp ./timelog.h  		${DESTDIR}/include/recommend
	 cp ./recsentence.h  	${DESTDIR}/include/recommend
	 cp ./ldainfer.h  		${DESTDIR}/include/recommend
	 cp ./recword.h  		${DESTDIR}/include/recommend
	 #cp ./dictionary.h  	${DESTDIR}/include/recommend
	 cp ./dictionary_new.h  ${DESTDIR}/include/recommend
	 #cp ./chseg.h  	  	${DESTDIR}/include/recommend
	 cp ./log.h  			${DESTDIR}/include/recommend
	 cp ./cachefile.h  		${DESTDIR}/include/recommend
	 cp ./librecommend.so  	${DESTDIR}/lib	

uninstall: librecommend.so 
	 -rm -rf ${DESTDIR}/include/recommend
	 -rm ${DESTDIR}/lib/librecommend.so

# Test
testchseg: test_chseg.cpp chseg.h chseg.cpp dictionary.h dictionary.cpp log.h log.cpp
	g++ -o testchseg  test_chseg.cpp  chseg.cpp  dictionary.cpp log.cpp

testrecword: test_recword.cpp recword.cpp dictionary_new.cpp recword.h dictionary_new.h 
	g++ -o testrecword  test_recword.cpp  recword.cpp  dictionary_new.cpp

testldainfer: test_ldainfer.cpp timelog.h timelog.cpp ldainfer.h ldainfer.cpp 
	g++ -o testldainfer  test_ldainfer.cpp timelog.cpp ldainfer.cpp 

testcachefile: cachefile.cpp test_cachefile.cpp log.cpp 
	g++ -o testcachefile  test_cachefile.cpp  log.cpp  cachefile.cpp

testeigen: testEigen.cpp  dictionary.cpp timelog.cpp
	g++ -I /home/david/David/matrix/eigen-eigen-6b38706d90a9/ testEigen.cpp \
		dictionary.cpp timelog.cpp  -o  eigen

testnewdict: test_dict.cpp dictionary.cpp dictionary.h dictionary_new.cpp dictionary_new.h
	g++ -g -o testnewdict test_dict.cpp  dictionary.cpp dictionary_new.cpp

createdict: create_dict.cpp dictionary.cpp dictionary.h 
	g++ -o createdict  create_dict.cpp  dictionary.cpp

createsenbin: create_newsentenceBin.cpp recsentence.h timelog.h cachefile.h
	g++ -o createsenbin create_newsentenceBin.cpp timelog.cpp cachefile.cpp

createldawords: create_ldawords.cpp
	g++ -o createldawords create_ldawords.cpp

testdb:  test_db.cpp timelog.h timelog.cpp
	g++ -o testdb test_db.cpp timelog.cpp -lkyotocabinet

.PHONY: clean
clean: 
	-rm librecommend.so 
	-rm *.o
