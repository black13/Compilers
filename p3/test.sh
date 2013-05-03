#!/usr/bin/env bash

if [ $# -ne 1 ]
then
	echo "Usage $0 testname"
	exit
fi

if [ ! -f samples/$1.decaf ]
then
	echo "Input $1.frag does not exist"
	exit 1
fi

if [ ! -f samples/$1.out ]
then
	echo "Output $1.out does not exist"
	exit 1
fi

./dcc < samples-checkpoint/$1.decaf &> samples-checkpoint/$1.test

DIFF=$(diff samples/$1.out samples/$1.test)

if [ "$DIFF" != "" ] 
then
  vimdiff samples-checkpoint/$1.out samples-checkpoint/$1.test
else
  echo "Test Passed!"
fi

rm samples-checkpoint/$1.test
