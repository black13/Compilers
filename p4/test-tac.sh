#!/bin/sh -f
#
# run
# Usage:  test-tac decaf-file
#

COMPILER=dcc

if [ $# -lt 1 ]; then
  echo "Run script error: The run script takes one argument, the path to a Decaf file."
  exit 1;
fi
if [ ! -x $COMPILER ]; then
  echo "Run script error: Cannot find $COMPILER executable!"
  echo "(You must run this script from the directory containing your $COMPILER executable.)"
  exit 1;
fi
if [ ! -r $1 ]; then
  echo "Run script error: Cannot find Decaf input file named '$1'."
  exit 1;
fi

./$COMPILER -d tac < $1 

exit 0;
