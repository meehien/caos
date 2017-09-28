#!/bin/bash

built = false

storeSize=32768
bufferSize=100
folderName=$(printf "test-%s" $storeSize)

oc=(100 250 500)

cd ../caos_client
if [ -e "caos_client" ]
then
	built = true
else
	make && built = true
fi

if [ built ]
then
	cp -f ../caos_testscripts/$folderName/store.bin .

	cd ../map_check
	make clean && make
	cp -f map_check ../caos_client
	cd ../caos_client

	total=0
	./map_check ../caos_testscripts/$folderName/csw.map > ../caos_testscripts/$folderName/OC$total

	for i in "${oc[@]}";
	do
		./caos_client --agent $i $bufferSize
		total=$(($total + $i))
		./map_check ../caos_testscripts/$folderName/csw.map > ../caos_testscripts/$folderName/OC$total
	done

	rm -f map_check
fi