#!/usr/bin/env bash

if [ $# -ne 1 ]
then
	echo "Usage $0 testname"
	exit
fi

if [ ! -f samples/$1.frag ]
then
	echo "Input $1.frag does not exist"
	exit 1
fi

if [ ! -f samples/$1.out ]
then
	echo "Output $1.out does not exist"
	exit 1
fi

./dcc < samples/$1.frag > samples/$1.test

vimdiff samples/$1.out samples/$1.test

