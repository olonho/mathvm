VM_ROOT      = ../../../..
ROOT         = .
PROGNAME     = lab2
USER_DEFS    = -ggdb3 -DPROGNAME="\"$(PROGNAME)\""
USER_OBJS    = $(OBJ)/mymain$(OBJ_SUFF) $(OBJ)/BytecodeTranslator$(OBJ_SUFF) $(OBJ)/BytecodeInterpreter$(OBJ_SUFF) 
USER_HEADERS = BytecodeInterpreter.h
RUN_TESTS    = $(VM_ROOT)/tests/run.py
BASH         = $(shell which bash)

include $(VM_ROOT)/common.mk

#LAB = $(BIN)/$(PROGNAME)
MVM = $(BIN)/mvm

all: $(MVM)

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJS) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJS) $(THREAD_LIB)

test: $(MVM)
	echo $(BASH)

	$(BASH) -c "if [[ !(-e tests) ]]; then ln -s $(VM_ROOT)/tests .; fi"
	$(RUN_TESTS)

#	unlink ./tests