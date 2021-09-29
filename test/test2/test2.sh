#!/bin/bash
echo "*********************************************"
echo "*************** AVVIO TEST 2 ****************"
echo "*********************************************"

#avvio il server
./server ./test/test2/configTest2/config.txt &
sleep 2s #aspetto che si sia avviato il server

#mi salvo il pid
SERVER_PID=$!

#TESTA RIMPIAZZAMENTO IN CACHE SERVER
./client -f "socket_name" -w testfile/lyric -D removedFile -p
./client -f "socket_name" -w testfile/monologhi -D removedFile -p
./client -f "socket_name" -w testfile/poesie -D removedFile -p
./client -f "socket_name" -w testfile/subdir2 -D removedFile -p
./client -f "socket_name" -R 5 -p -d downloadTest2

sleep 2s
#sighup al server
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID

echo "*********************************************"
echo "************** TEST 2 SUPERATO **************"
echo "*********************************************"
exit 0
