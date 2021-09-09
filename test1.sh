#!/bin/bash

echo "Avvio script client"
pid=$!
sleep 2s #aspetto che si sia avviato il server

#TESTA GESTIONE MEMORIA SERVER
./client -f "socket_name" -p -t 200 -W testfile/prova4.txt,testfile/big.txt,testfile/monologhi/Joker.txt -w testfile/poesie,3 -r /Users/gorsu/Desktop/SOLProject21/SOLProject21/testfile/monologhi/Joke.txt,/Users/gorsu/Desktop/SOLProject21/SOLProject21/testfile/big.txt -R 2 -d downloadTest1 -c /Users/gorsu/Desktop/SOLProject21/SOLProject21/testfile/monologhi/Joker.txt

sleep 1s
#sighup al server
killall SIGHUP $pid
wait $pid
