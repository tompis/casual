#!/bin/sh
swig -java src/java_tx.i 
swig -java src/java_xa.i
swig -java src/java_xatmi.i
swig -java src/java_xatmi_server.i
casual-make compile
casual-make link
casual-make install
javac -d obj/class src/*.java
jar -cf bin/casual-java.jar obj/class/*
#mcs -target:library -out:bin/libcasual-mono.dll src/*.cs 

