#!/bin/bash
echo "*********************************************"
echo "*************** AVVIO TEST 3 ****************"
echo "*********************************************"

#avvio il server
./server ./test/test3/configTest3/config.txt &
sleep 2s #aspetto che si sia avviato il server

#mi salvo il pid
SERVER_PID=$!

secs=30                           # tempo massimo di esecuzione del server
endTime=$(( $(date +%s) + secs )) # Calcola quando terminare
n=0
i=1

while [ $(date +%s) -lt $endTime ]; do
    ./client -f "socket_name" -W testfile/monologhi/Joker.txt,testfile/prova4.txt -l /home/xubuntu/Scrivania/SOLProject21/testfile/monologhi/Joker.txt -r /home/xubuntu/Scrivania/SOLProject21/testfile/monologhi/Joker.txt -u /home/xubuntu/Scrivania/SOLProject21/testfile/monologhi/Joker.txt -c /home/xubuntu/Scrivania/SOLProject21/testfile/monologhi/Joker.txt,/home/xubuntu/Scrivania/SOLProject21/testfile/prova4.txt
    (( n++ ))
    (( i++ ))
done
#TESTA GESTIONE MEMORIA SERVER


sleep 2s
#sighup al server
kill -s SIGINT $SERVER_PID
wait $SERVER_PID

for((i=0;i<n;++i)); do
    wait ${PID[i]}
done

echo "*********************************************"
echo "************** TEST 3 SUPERATO **************"
echo "*********************************************"
exit 0
