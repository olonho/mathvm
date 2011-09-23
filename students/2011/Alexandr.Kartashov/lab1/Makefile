VM_ROOT = ../../../..
ROOT   = .
USER_DEFS = -ggdb3
USER_OBJ = $(OBJ)/mymain$(OBJ_SUFF) $(OBJ)/mytranslator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB1 = $(BIN)/lab1

all: $(LAB1)

$(LAB1): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB)

test: $(LAB1)
	./test.sh
