gem5:
	./gem5_build.sh

SRC := $(wildcard *.py)
TEST := $(patsubst %.py,%,${SRC})

# DEBUG ?= O3PipeView,O3CPUAll
# konata toy.out
DEBUG ?= Toy

${TEST}:%:%.py
	../../build/RISCV/gem5.opt --debug-flag=${DEBUG} --debug-file=toy.out $^
