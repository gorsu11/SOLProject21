#!/bin/bash

if [ $# -eq 0 ]
then
    echo "Inserire pathname del file log"
    exit 1
fi

#Stampa numero di scritture
num_writeFile=$(grep "Esito inserisciDati: POSITIVO" $1 | grep -v "/" | wc -l)
echo -e "Numero di writeFile: $num_writeFile"

#Stampa numero di append
num_appendFile=$(grep "Esito appendDati: POSITIVO" $1 | grep -v "/" | wc -l)
echo "Numero di appendFile: $num_appendFile"
echo -e "Numero totale di scritture: $(($num_writeFile + $num_appendFile))\n"

#Stampa numero di close
num_closeFile=$(grep "Esito rimuoviCliente: POSITIVO" $1 | grep -v "/" | wc -l)
echo "Numero di closeFile: $num_closeFile"

#Stampa numero di read
num_readFile=$(grep "Esito prendiFile: POSITIVO" $1 | grep -v "/" | wc -l)
echo -e "Numero totale di read: $(($num_readFile))\n"

#Stampa numero di lock
echo -n "Numero totale di lockFile: "
grep "Esito bloccaFile: POSITIVO" $1 | grep -v "/" | wc -l

#Stampa numero di open-lock
echo -n "Numero totale di open-lock: "
grep "Operazione: openlockFile" $1 | grep -v "/" | wc -l

#Stampa numero di unlock
echo -n "Numero totale di unlockFile: "
grep "Esito sbloccaFile: POSITIVO" $1 | grep -v "/" | wc -l

#stampa il numero di replace
num_replace=$(grep "replace" $1 | grep -v "/" | wc -l)
echo -e "\nNumero totale di file espulsi: $num_replace\n"

#Somma il numero di bytes scritti sulla cache
tot_bytes_writes=$(grep -Eo "Bytes scritti sulla cache: [0-9]+" $1 | grep -Eo "[0-9]+" | { sum=0; while read num; do ((sum+=num)); done; echo $sum; } )

#Stampa numero di bytes scritti
#se non è stato trovato nessun valore ==> num_totalCache=0
if [ -z "$tot_bytes_writes" ]; then
    tot_bytes_writes=0
fi
#Stampa il numero medio bytes scritti
echo "Numero totale di bytes scritti: $tot_bytes_writes"

if [ ${num_writeFile} -gt 0 ]; then
    mean_bytes_written=$(echo "scale=4; ${tot_bytes_writes} / ${num_writeFile}" | bc -l)
    echo -e "Media dei bytes scritti: ${mean_bytes_written}\n"
fi


#Stampa numero di bytes letti
tot_bytes_reads=$(grep -Eo "Bytes letti dal file: [0-9]+" $1 | grep -Eo "[0-9]+" | { sum=0; while read num; do ((sum+=num)); done; echo $sum; } )
#se non è stato trovato nessun valore ==> num_totalCache=0
if [ -z "$tot_bytes_reads" ]; then
    tot_bytes_reads=0
fi
#Stampa il numero medio e totale di bytes letti
echo "Numero totale di bytes letti: $tot_bytes_reads"

if [ ${num_readFile} -gt 0 ]; then
        media_bytesLetti=$(echo "scale=4; ${tot_bytes_reads} / ${num_writeFile}" | bc -l)
    echo "Media dei bytes letti: ${media_bytesLetti}"
fi

#Stampa numero totale di file creati
echo -n "Numero totale file creati: "
grep "creaFile" $1 | wc -l
echo ""


#Stampa numero di richieste per ogni thread
for i in $(grep -Eo 'Thread Worker: \w+' $1 | grep -v "/" | cut -d " " -f3 | sort -n -u); do

    echo -n "Numero di Richieste del thread $i: "
    grep "$i" $1 | wc -l

done
echo ""

maxCon=0
currentCon=0

while read -r line ; do
    #echo "Processing:              $line"
    if grep -q "connessione chiusa:" <<< "$line" ; then
        ((currentCon--))
    elif grep -q "Nuova connessione:" <<< "$line" ; then
        ((currentCon++))
        if [ "$currentCon" -gt "$maxCon" ]; then
            maxCon=$currentCon
        fi
    fi
done < <(grep "connessione" $1 | grep -v "/") #sulla VM "done <<< $(grep "connessione" $1 | grep -v "/")" non funziona

echo -e "Massimo numero di connessioni contemporanee: $maxCon\n"

max_files=$(grep -Eo "Numero di file massimo: [0-9]+" $1 | grep -Eo "[0-9]+")
echo "Numero massimo di file presenti: $max_files"

max_bytes=$(echo "scale=6; $(grep -Eo "Dimensione massima di bytes: [0-9]+" $1 | grep -Eo "[0-9]+") /1048576"| bc -l)
echo -e "Dimensione massima della cache: $max_bytes Bytes\n"
