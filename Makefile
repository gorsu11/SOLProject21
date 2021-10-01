CC 		= gcc
CFLAGS		= -g -Wall
TARGETS		= server client

.PHONY: all clean cleanall test1 test2 test3

#GENERA ESEGUIBILI SERVER E CLIENT
all : $(TARGETS)

server : src/server.c lib/libParsing.a lib/libFunction.a lib/libLog.a
	$(CC) $(CFLAGS) $< -o $@ -Llib -lParsing -Llib -lFunction -Llib -lLog -lpthread

client : src/client.c lib/libApi.a lib/libCommand.a
	$(CC) $(CFLAGS) src/client.c -o $@ -Llib -lApi -Llib -lCommand

objs/parsingFile.o : src/parsingFile.c
	$(CC) $(CFLAGS) -c src/parsingFile.c -o $@

objs/log.o: src/log.c
	$(CC) $(CFLAGS) -c src/log.c -o $@

objs/serverFunction.o : src/serverFunction.c
	$(CC) $(CFLAGS) -c src/serverFunction.c -o $@

objs/commandList.o : src/commandList.c
	$(CC) $(CFLAGS) -c src/commandList.c -o $@

objs/interface.o : src/interface.c
	$(CC) $(CFLAGS) -c src/interface.c -o $@

lib/libParsing.a: objs/parsingFile.o
	ar r lib/libParsing.a objs/parsingFile.o

lib/libLog.a: objs/log.o
	ar r lib/libLog.a objs/log.o

lib/libFunction.a: objs/serverFunction.o
	ar r lib/libFunction.a objs/serverFunction.o

lib/libCommand.a : objs/commandList.o
	ar r lib/libCommand.a objs/commandList.o

lib/libApi.a : objs/interface.o 
	ar r lib/libApi.a objs/interface.o

#ELIMINA SOLO GLI ESEGUIBILI
clean :
	-rm -f $(TARGETS)

#ELIMINA I FILE ESEGUIBILI, OGGETTO E TEMPORANEI
cleanall :
	-rm -f $(TARGETS) objs/*.o lib/*.a #tmp/* *~

#LANCIA IL PRIMO TEST
test1 :
	./test/test1/test1.sh
	./statistiche/statistiche.sh ./test/test1/log/log1.txt

#LANCIA SECONDO TEST
test2 :  
	./test/test2/test2.sh
	./statistiche/statistiche.sh ./test/test2/log/log2.txt 	

#LANCIA TERZO TEST
test3 : 
	./test/test3/test3.sh
	./statistiche/statistiche.sh ./test/test3/log/log3.txt