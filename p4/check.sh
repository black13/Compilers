#!/usr/bin/env bash
clean=false
enable_diff=false

if $clean ; then
    make clean
fi
make

[ -x dcc ] || { echo "Error: dcc not executable"; exit 1; }


#timeout code
function timeout {
    declare -i timeout=10
    declare -i interval=1
    declare -i delay=1
    (
        ((t = timeout))

        while ((t > 0)); do
            sleep 0.1
            killall -0 spim || exit 0
            #killall -0 dcc || exit 0
            ((t -= interval))
        done

        # Be nice, post SIGTERM first.
        # The 'exit 0' below will be executed if any preceeding command fails.
        #echo "Timeout Detected!"
        #kill -s SIGTERM $$ && kill -0 $$ || exit 0
        #sleep $delay
        #kill -s SIGKILL $$
        killall dcc
        killall spim
    ) 2> /dev/null &

    exec "$@"
}


LIST=
if [ "$#" = "0" ]; then
  LIST=`ls samples/*.out`
else
  for test in "$@"; do
    LIST="$LIST samples/$test.out"
    enable_diff=true
  done
fi

total_tests=0
pass_tests=0
fail_tests=0

for file in $LIST; do
  base=`echo $file | sed 's/\(.*\)\.out/\1/'`

  ext=''
  if [ -r $base.frag ]; then
    ext='frag'
  elif [ -r $base.decaf ]; then
    ext='decaf'
  else
    echo "Error: Input file for base: $base not found"
    exit 1
  fi

  cut_offset=7
  trim_offset=6

  if [ "$base" == "samples/link1" -o "$base" == "samples/link3" ]; then
      cut_offset=4
      trim_offset=0
  fi

  #tail -n +$trim_offset $file > $file.trim
  tmp=${TMP:-"./samples/"}/check.tmp
  if [ ! -r $base.in ]; then
    timeout ./_run $base.$ext 2>&1 | tail -n +$cut_offset > $tmp
  else
    timeout ./_run $base.$ext < $base.in 2>&1 | tail -n +$cut_offset > $tmp
  fi

  printf "Checking %-27s: " $base.$ext
  if ! cmp -s $tmp $file.trim; then
    let fail_tests++
    echo "FAIL <--"
    if $enable_diff ; then
        vimdiff $tmp $file.trim
    fi
  else
    let pass_tests++
    echo "PASS"
  fi
  let total_tests++
done
rm ./samples/check.tmp

echo "Fail: $fail_tests/$total_tests"
echo "Pass: $pass_tests/$total_tests"
