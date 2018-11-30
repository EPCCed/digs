#!/bin/sh

javac uk/ac/ed/epcc/qcdgrid/browser/DatabaseDrivers/*.java
javac uk/ac/ed/epcc/qcdgrid/browser/*.java
javac uk/ac/ed/epcc/qcdgrid/*.java
javac uk/ac/ed/epcc/qcdgrid/metadata/validator/*.java
javac uk/ac/ed/epcc/qcdgrid/metadata/*.java
javac uk/ac/ed/epcc/qcdgrid/datagrid/*.java
javac uk/ac/ed/epcc/qcdgrid/browser/GUI/*.java
javac uk/ac/ed/epcc/qcdgrid/browser/ResultHandlers/*.java
javac uk/ac/ed/epcc/qcdgrid/browser/TemplateHandler/*.java
javac uk/ac/ed/epcc/qcdgrid/browser/QueryHandler/*.java
javac uk/ac/ed/epcc/qcdgrid/browser/Configuration/*.java

gcc -shared -o jni/libQCDgridJavaClient.so jni/QCDgridClient.c -L../ -lqcdgridclient -I$JAVA_HOME/include -I$JAVA_HOME/include/linux

echo "Done."
