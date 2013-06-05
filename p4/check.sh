#! /bin/sh
clean=true
enable_diff=false

if $clean ; then
    make clean
fi
make

[ -x dcc ] || { echo "Error: dcc not executable"; exit 1; }

LIST=
if [ "$#" = "0" ]; then
  LIST=`ls samples/*.out`
else
  for test in "$@"; do
    LIST="$LIST samples/$test.out"
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
      cut_offset=3
      trim_offset=0
  fi

  #tail -n +$trim_offset $file > $file.trim
  tmp=${TMP:-"./samples/"}/check.tmp
  if [ ! -r $base.in ]; then
    ./timeout4 ./_run $base.$ext 2>&1 | tail -n +$cut_offset > $tmp
  else
    ./timeout4 ./_run $base.$ext < $base.in 2>&1 | tail -n +$cut_offset > $tmp
  fi

  printf "Checking %-27s: " $file
  if ! cmp -s $tmp $file.trim; then
    let fail_tests++
    echo "FAIL <--"
    if $enable_diff ; then
        diff $tmp $file.trim
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
