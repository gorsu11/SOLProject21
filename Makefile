CC 		= gcc
CFLAGS		= -g -Wall
TARGETS		= server client

.PHONY: all clean cleanall test1 test2 test3

#GENERA ESEGUIBILI SERVER E CLIENT
all : $(TARGETS)

server : src/server.c
	$(CC) $(CFLAGS) $< -o $@ -lpthread

client : src/client.c lib/libapi.a lib/libCommand.a
	$(CC) $(CFLAGS) src/client.c -o $@ -Llib -lapi -Llib -lCommand

objs/commandList.o : src/commandList.c
	$(CC) $(CFLAGS) -c src/commandList.c -o $@

objs/interface.o : src/interface.c
	$(CC) $(CFLAGS) -c src/interface.c -o $@

lib/libCommand.a : objs/commandList.o
	ar r lib/libCommand.a objs/commandList.o

lib/libapi.a : objs/interface.o 
	ar r lib/libapi.a objs/interface.o

#ELIMINA SOLO GLI ESEGUIBILI
clean :
	-rm -f $(TARGETS)

#ELIMINA I FILE ESEGUIBILI, OGGETTO E TEMPORANEI
cleanall :
	-rm -f $(TARGETS) objs/*.o lib/*.a #tmp/* *~

#LANCIA IL PRIMO TEST
test1 : $(TARGETS)
	./server configTest1/config.txt &
	chmod +x test1.sh 
	./test1.sh &

#LANCIA SECONDO TEST
test2 : $(TARGETS)
	./server configTest2/config.txt &
	chmod +x test2.sh 
	./test2.sh &	

#LANCIA TERZO TEST
test3 : $(TARGETS)
	./server configTest3/config.txt &
	chmod +x test3.sh 
	./test3.sh &	
	
