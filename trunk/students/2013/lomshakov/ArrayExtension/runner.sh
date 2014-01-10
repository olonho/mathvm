script="./../../../../tests/run.py"
running="./ArrayExtension/build/debug/mvm"

if [[ $1 == *m* ]]; then
  make
fi
if [[ $1 == *t* ]]; then
  $script -e $running -t ./../../../../tests/
  $script -e $running -t ./ext_tests/
  #$script -e $running -t ./../../../../tests/additional
  #$script -e $running -t ./../../../../tests/optional
  #$script -e $running -t ./ext_tests/
fi

