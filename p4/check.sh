#! /bin/sh
make clean
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
        
  #tail -n +6 $file > $file.trim
  tmp=${TMP:-"./samples/"}/check.tmp
  ./_run $base.$ext 2>1 | tail -n +7 > $tmp

  printf "Checking %-27s: " $file
  if ! cmp -s $tmp $file.trim; then
    echo "FAIL <--"
   # diff $tmp $file.trim
  else
    echo "PASS"
  fi
done
rm ./samples/check.tmp
