#!/bin/bash

cd ../cgen
g++ skel.cpp -o skel
g++ cgen.cpp lexer.cpp parse.cpp -o cgen
g++ acta.cpp -o acta

mv skel ../bin/skel
mv cgen ../bin/cgen
mv acta ../bin/acta

chmod 755 ../bin/skel
chmod 755 ../bin/cgen
chmod 755 ../bin/acta
chmod 755 ../bin/everest.sh
