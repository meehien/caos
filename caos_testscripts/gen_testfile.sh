#!/bin/sh

dd if=/dev/zero of=test${1}.data bs=1024 count=${1}
#fallocate -l ${1} test${1}.data
