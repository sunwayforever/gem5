gem5:
	./gem5_build.sh

SRC := $(wildcard *.py)
TEST := $(patsubst %.py,%,${SRC})

DEBUG ?= Toy

simple:DEBUG := O3PipeView,O3CPUAll
simple_with_l1cache:DEBUG := O3PipeView,O3CPUAll
# konata toy.out

${TEST}:%:%.py
	../../build/RISCV/gem5.opt --debug-flag=${DEBUG} --debug-file=toy.out $^
