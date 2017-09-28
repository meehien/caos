#!/bin/bash

built = false

storeSize=32768
fileSize=10
folderName=$(printf "test-%s" $storeSize)
fileName=$(printf "test%s.data" $fileSize)

cd ../caos_client

mkdir $folderName

../caos_testscripts/gen_testfile.sh $fileSize

if [ -e "caos_client" ]
then
	built = true
else
	make && built = true
fi

if [ built ]
then
	./caos_client --writer --add $fileName

	echo "finished adding"

	for i in `seq 1 10`;
	do
		./caos_client --writer --get $fileName resFile.data
		echo "finished read $i"
	done

	rm resFile.data
	rm $fileName

	mv csw.map $folderName
	cp ../caos_server/store.bin $folderName

	mv $folderName ../caos_testscripts
fi