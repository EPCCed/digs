#!/bin/sh
gcc -shared -fPIC -o libQCDgridJavaClient.so QCDgridClient.c -lqcdgridclient -L../.. -I$JAVA_HOME/include -I$JAVA_HOME/include/linux

