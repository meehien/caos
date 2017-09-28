#!/bin/sh

dd if=/dev/zero of=test${1}.data bs=1024 count=0 seek=$[1024*${1}]

