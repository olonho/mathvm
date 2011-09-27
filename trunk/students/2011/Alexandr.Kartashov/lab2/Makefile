VM_ROOT   = ../../../..
ROOT      = .
PROGNAME  = lab2
USER_DEFS = -ggdb3 -DPROGNAME="\"$(PROGNAME)\""
USER_OBJ  = $(OBJ)/mymain$(OBJ_SUFF) $(OBJ)/BytecodeTranslator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB = $(BIN)/$(PROGNAME)

all: $(LAB)

$(LAB): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB)

test: $(LAB)
	$(LAB) $(VM_ROOT)/tests/add.mvm