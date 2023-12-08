#!/bin/sh
mamba init
mamba activate everest
curl https://bootstrap.pypa.io/pip/2.7/get-pip.py -o get-pip.py
python2 get-pip.py
rm get-pip.py
python2 -m pip install tornado==4.5.3
