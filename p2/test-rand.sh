#!/bin/bash

if [ "$1" == "--help" ]
then
  echo "test-rand.sh [OPTIONS]"
  echo "-r N run n number of random test cases, default=24"
  echo "-s SEED set the seed value, default is random"
  echo "-l L set maximum size of test cases, default=300"
  exit 0
fi

if [[ ! -f "dcc" || ! -f "rmutt" || ! -f "decaf.rm" ]]
then
  echo "requires 'dcc' and 'rmutt' and 'decaf.rm'"
  exit 1
fi

REPEAT=24
LENGTH=300
SEED=$RANDOM

while getopts hr:l:s: opt; do
  case $opt in
    h) echo "test-rand.sh [OPTIONS]"
       echo "-r N run n number of random test cases, default=24"
       echo "-s SEED set the seed value, default is random"
       echo "-l L set maximum size of test cases, default=300"
       exit 0;;
    r) REPEAT=$OPTARG;;
    s) SEED=$OPTARG;;
    l) LENGTH=$OPTARG;;
  esac
done

if [ ! -d random_samples ]
then
  echo "creating random samples directory..."
  mkdir random_samples/
fi

cat /dev/null > result
rm -f random_samples/*decaf random_samples/*out

echo "start generating random samples with random seed = $SEED"
echo

for ((test_case_id=1,syntax_errors=0; test_case_id<=$REPEAT; test_case_id++,SEED++))
do
  test_case_name=random_samples/rand$test_case_id
  ./rmutt -r $SEED decaf.rm | indent -bad -br -npcs > $test_case_name.decaf 2> /dev/null
  sed -i 's/\(]\)\([a-zA-Z]\)/] \2/g' $test_case_name.decaf

  entropy=`wc -w $test_case_name.decaf | awk '{print $1}'`
  if [ $entropy -gt $LENGTH ]
  then
    (( test_case_id-- ))
    seed=$RANDOM
  else
    ./dcc < $test_case_name.decaf &> $test_case_name.out
    ls -l $test_case_name.out

    match=$(grep "syntax error" $test_case_name.out)
    if [ -n "$match" ]
    then
      (( syntax_errors++ ))
    fi
  fi
done
echo "finished generating random samples"

echo
echo "$(( $REPEAT - $syntax_errors )) samples, no syntax errors: " >> result
grep -rv "syntax error" random_samples/*out | awk -F':' '{print $1}' | uniq >> result
echo >> result
echo "$syntax_errors samples with syntax errors: " >> result
grep -r "syntax error" random_samples/*out | awk -F':' '{print $1}' | uniq >> result
echo >> result
cat result
ls -l result

