#!/bin/bash
set -e

cd ../caos_client
while true; do
	./caos_client $2 --agent 1 $1
	sleep 1
done
