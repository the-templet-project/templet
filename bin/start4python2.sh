#!/bin/bash
BIN_DIR=`dirname "$(cd ${0%/*} && echo $PWD/${0##*/})"`
AGENT_HOME=`dirname "$BIN_DIR"`
cd $AGENT_HOME

PID_FILE=agent.pid

if [ -f $PID_FILE ] && [ -f /proc/$(cat $PID_FILE)/status ]; then
   echo "Agent is already running"
else
#    if which python3 > /dev/null; then
#        PYTHON=python3
#    else
        # Find latest Python 2.x version
        PYTHON=$(ls /usr/bin/python2.? | tail -n1)
#    fi
    # Make sure Python is 2.6 or later
    PYTHON_OK=`$PYTHON -c 'import sys; print (sys.version_info >= (2, 6) and "1" or "0")'`
    if [ "$PYTHON_OK" = '0' ]; then
        echo "Python versions < 2.6 are not supported"
        exit 1
    fi
    PYTHONPATH=$(dirname $0)/..:$PYTHONPATH $PYTHON -m everest_agent.start 2>errors.txt $@ &
    echo $! > $PID_FILE
    echo "Agent is started"
fi
