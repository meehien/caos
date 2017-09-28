#!/bin/bash

cd ../caos_server
if [ -e "caos_server" ]
then
	./caos_server --listen
else
	make && ./caos_server --listen
fi
