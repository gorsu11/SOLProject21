#!/bin/bash
echo "*********************************************"
echo "*************** AVVIO TEST 1 ****************"
echo "*********************************************"

#avvio il server
./server configTest1/config.txt &
sleep 2s #aspetto che si sia avviato il server

#mi salvo il pid
SERVER_PID=$!

#TESTA GESTIONE MEMORIA SERVER
./client -f "socket_name" -p -t 200 -W testfile/prova4.txt,testfile/big.txt,testfile/monologhi/Joker.txt -w testfile/poesie,3 -r /Users/gorsu/Desktop/SOLProject21/SOLProject21/testfile/monologhi/Joker.txt,/Users/gorsu/Desktop/SOLProject21/SOLProject21/testfile/big.txt -R 2 -d downloadTest1 -c /Users/gorsu/Desktop/SOLProject21/SOLProject21/testfile/monologhi/Joker.txt

sleep 2s
#sighup al server
kill -s SIGHUP $SERVER_PID
wait $SERVER_PID

echo "*********************************************"
echo "************** TEST 2 SUPERATO **************"
echo "*********************************************"
exit 0

