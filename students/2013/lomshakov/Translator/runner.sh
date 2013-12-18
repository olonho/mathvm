script="./../../../../tests/run.py"

if [[ $1 == *r* ]]; then
  running="./build/opt/mvm"
  make clean
  make OPT=1
fi

if [[ $1 == *d* ]]; then
  running="./build/debug/mvm"
  make clean
  make
fi

if [[ $1 == *t* ]]; then
  $script -e $running -t ./../../../../tests/
  #$script -e $running -t ./../../../../tests/optional
  #$script -e $running -t ./../../../../tests/additional
fi

