#!/usr/bin/env bash

JOBS=12
PREFIX="invalid"

while [ "$1" != "" ]; do
  case $1 in
    -j | --jobs  ) shift
                   JOBS=$1
                   ;;
    -p | --prefix) shift
                   PREFIX=$1
                   ;;
    * )            echo "Invalid arguments"
                   exit 1
esac
shift
done

if [ "${PREFIX,,}" = "invalid" ]; then
  echo "Must specify PREFIX dir"
  exit 1
fi

set -e
set -o xtrace
start_dir=$(pwd)
trap 'cd $start_dir' EXIT

# Must execute from the directory containing this script
cd "$(dirname "$0")"


######################

./deps.sh -d -s -p "$PREFIX" -j $JOBS -c 1 -e 5
cp "$PREFIX/lib/cmake/z3/Z3Targets-debug.cmake" .

./deps.sh -rdi -a -p "$PREFIX" -j $JOBS -c 1 -e 5
cp "Z3Targets-debug.cmake" "$PREFIX/lib/cmake/z3/"

./deps.sh -rdi -a -p "$PREFIX" -j $JOBS -c 6 -e 7

