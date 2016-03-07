#!/bin/bash

EXE=./iec-runtime\ ./io-task\ ./sv-task
NFSDIR=/home/mynfs/iec-runtime/
TESTDIR=./test/
GENOBJ=./trans

# Step1: rebuild iec-runtime
make rebuild
# Step2: generate obj file
# ${GENOBJ} "$1" < "$1".test
${GENOBJ} $1
# Step3: move iec-runtime & obj file to NFS directory
mv ${EXE} ${NFSDIR}
mv "$1".obj exe.obj
mv exe.obj ${NFSDIR}
# Step4: clean up
make clean
