VM_ROOT      = ../../../..
ROOT         = .
PROGNAME     = lab2
USER_DEFS    = -ggdb3 -DPROGNAME="\"$(PROGNAME)\""
USER_OBJS    = $(OBJ)/mymain$(OBJ_SUFF) $(OBJ)/BytecodeTranslator$(OBJ_SUFF) $(OBJ)/BytecodeInterpreter$(OBJ_SUFF) 
USER_HEADERS = BytecodeInterpreter.h
RUN_TESTS    = $(VM_ROOT)/tests/run.py
BASH         = $(shell which bash)

include $(VM_ROOT)/common.mk

MVM = $(BIN)/mvm

all: $(MVM)

ast-patched:
	patch -d $(VM_ROOT)/include -p2 < ast.patch
	touch ast-patched

$(MVM): ast-patched $(OUT) $(MATHVM_OBJ) $(USER_OBJS) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJS) $(THREAD_LIB)

tests:
	ln -s $(VM_ROOT)/tests .

test: tests $(MVM)
	$(RUN_TESTS)