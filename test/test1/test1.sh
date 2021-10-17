#!/bin/bash
echo "*********************************************"
echo "*************** AVVIO TEST 1 ****************"
echo "*********************************************"

#avvio il server
valgrind --leak-check=full ./server ./test/test1/configTest1/config.txt &
sleep 2s #aspetto che si sia avviato il server

#mi salvo il pid
SERVER_PID=$!

#TESTA GESTIONE MEMORIA SERVER
./client -f "socket_name" -p -t 200 -W testfile/prova4.txt,testfile/big.txt,testfile/monologhi/Joker.txt -w testfile/subdir2,3 -r /home/xubuntu/Scrivania/SOLProject21/testfile/monologhi/Joker.txt,/home/xubuntu/Scrivania/SOLProject21/testfile/big.txt -R 2 -d downloadTest1 -l /home/xubuntu/Scrivania/SOLProject21/testfile/big.txt -c /home/xubuntu/Scrivania/SOLProject21/testfile/monologhi/Joker.txt -u /home/xubuntu/Scrivania/SOLProject21/testfile/big.txt

sleep 2s
#sighup al server
kill -s SIGHUP $SERVER_PID memcheck-amd64-
wait $SERVER_PID

echo "*********************************************"
echo "************** TEST 1 SUPERATO **************"
echo "*********************************************"
exit 0
