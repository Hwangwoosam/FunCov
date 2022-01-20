#!/bin/bash

SRC=$(pwd)

SANITIZER="address"
FUZZING_ENGINE="libfuzzer"
ARCHITECTURE="x86_64"
LIB_FUZZING_ENGINE="/usr/lib/libFuzzingEngine.a"

export AFL_LLVM_MODE_WORKAROUND=0
export AFL_ENABLE_DICTIONARY=0
export AFL_ENABLE_CMPLOG=1
export AFL_LAF_CHANCE=3
