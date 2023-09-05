#!/bin/bash
pushd ../../
scons build/RISCV/gem5.opt -j 36 --linker=gold
popd
