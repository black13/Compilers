#!/bin/bash
make clean
make

rm -f samples/*.mine
#cat /dev/null > out

for file in `ls samples/*.decaf`
	do
                filename=$(basename "$file")
		extension="${filename##*.}"
		testcasename="${filename%.*}"

		my_output=samples/"$testcasename".mine
#                frag=samples/"$testcasename".frag
		their_output=samples/"$testcasename".out

                dos2unix --quiet $file # converts files in place
                dos2unix --quiet $their_output #use -n option to save separate

		cat /dev/null > $my_output
                ./dcc < $file &> $my_output

		differences=$(diff ${my_output} ${their_output})

		if [ -n "$differences" ]; then
			echo
			#echo "Difference found in testcase $testcasename ($my_output vs $their_output): "
			#echo "$differences" | tee out
			#echo
			#echo "See the out file"
      vimdiff $my_output $their_output
			exit 1
		fi
	done

echo "It seems to be okay"
