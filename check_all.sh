#!/bin/bash

function build() {
  echo "=> Building with options [$@]..." && echo
  ./configure.py $@
  ./run_tests.sh
  echo
  echo "=> Done config [$@]."
}

build
build --san address,undefined

build --config release
build --config release --san address,undefined
