#! /bin/bash

PATH__=./test/astprint/
FILES_MVM=${PATH__}*.mvm
echo $FILES_MVM

for f in $FILES_MVM
do	
	echo ""
  	echo "Processing $f file..."
	name="$(echo ${f%*.mvm})"".expected"
	# echo "$name"
	tmp_file=${f:16}
	echo "$tmp_file"
	./build/debug/mvm -p  $f > "$tmp_file"
	# echo "diff ${name} ${tmp_file}"
	res=$(diff ${name} ${tmp_file})
	if [ !  -z "${res}" ]; then
		echo "FAIL" 
		echo "${res}"
	else
		echo "OK"
	fi
	
  # take action on each file. $f store current file name
  # cat $f
done

