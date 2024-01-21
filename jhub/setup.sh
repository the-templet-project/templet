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

cd ..
git clone https://gitlab.com/everest/agent.git ./everest_agent

mkdir ./everest_agent/conf 
cp    ./everest_agent/everest_agent/agent.conf.default ./everest_agent/conf/agent.conf

cp    ./bin/start4python2.sh ./everest_agent/bin/start.sh
sed -i -e 's=/srv/conda/envs/everest/bin/python2=/opt/tljh/user/envs/everest/bin/python2=' ./everest_agent/bin/start.sh
chmod 755 ./everest_agent/bin/start.sh
