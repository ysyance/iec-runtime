#!/bin/bash

EXE=./iec-runtime\ ./io-task\ ./sv-task
NFSDIR=/home/mynfs/iec-runtime/
TESTDIR=./sample
GENOBJ=./translator.exe


# Step1: rebuild iec-runtime
make rebuild
# make
# Step2: generate obj file
# ${GENOBJ} "$1" < "$1".test
g++ translator.cc -std=c++11 -o translator.exe
if [ $# -gt 0 ]; then
	${GENOBJ} ${TESTDIR}/"$1".test
	# Step3: move iec-runtime & obj file to NFS directory
	mv ${EXE} ${NFSDIR}
	# mv "$1" exec.obj
	mv exec.obj ${NFSDIR}
	# Step4: clean up
	make clean
	echo "PLC OneKey: Success !"
else 
	# Step3: move iec-runtime & obj file to NFS directory
	mv ${EXE} ${NFSDIR}
	# Step4: clean up
	make clean
	echo "lack of argument: there are only $# arguments !!!"
fi
