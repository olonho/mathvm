script="./../../../../tests/astprint/run.py"
running="./build/debug/mvm"

if [[ $1 == *m* ]]; then
  make
fi
if [[ $1 == *t* ]]; then
  $script -e $running -d ./../../../../tests/
  $script -e $running -d ./../../../../tests/optional
  $script -e $running -d ./../../../../tests/additional
fi

