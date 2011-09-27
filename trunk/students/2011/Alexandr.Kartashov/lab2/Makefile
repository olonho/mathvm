VM_ROOT      = ../../../..
ROOT         = .
PROGNAME     = lab2
USER_DEFS    = -ggdb3 -DPROGNAME="\"$(PROGNAME)\""
USER_OBJS    = $(OBJ)/mymain$(OBJ_SUFF) $(OBJ)/BytecodeTranslator$(OBJ_SUFF) $(OBJ)/BytecodeInterpreter$(OBJ_SUFF) 
USER_HEADERS = BytecodeInterpreter.h

include $(VM_ROOT)/common.mk

LAB = $(BIN)/$(PROGNAME)

all: $(LAB)

$(LAB): $(OUT) $(MATHVM_OBJ) $(USER_OBJS) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJS) $(THREAD_LIB)

test: $(LAB)
	$(LAB) $(VM_ROOT)/tests/add.mvm