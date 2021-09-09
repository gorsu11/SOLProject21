#!/bin/bash
sleep 1 #aspetto che si sia avviato il server
echo "Avvio script client"
#TESTA RIMPIAZZAMENTO IN CACHE SERVER
./client -f "socket_name" -w testfile/lyric -D removedFile -p
./client -f "socket_name" -w testfile/monologhi -D removedFile -p
./client -f "socket_name" -w testfile/poesie -D removedFile -p
./client -f "socket_name" -w testfile/subdir2 -D removedFile -p
./client -f "socket_name" -R 5 -p -d downloadTest2
#sighup al server
killall -s SIGHUP server
