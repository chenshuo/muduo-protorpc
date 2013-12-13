#!/bin/sh

set -x
export PYTHONPATH=../zurg/client
python nqueens_cli.py $*
