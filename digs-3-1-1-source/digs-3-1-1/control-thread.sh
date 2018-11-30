#!/bin/sh
#
procfile="procs.txt"
grepfile="grep.txt"
ps -ax > $procfile
grep background $procfile > $grepfile 
if [ -s $grepfile ]; then
  echo "Control thread process may already be started. Please kill any such processes."
  exit 0
else
  echo "No dead control thread processes detected, proceeding."
fi
rm $procfile $grepfile

grid-proxy-init

background $*

while [ $? -eq 0 ] ;
do
  grid-proxy-init
  background $*
done
