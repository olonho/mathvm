ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3 -std=c++0x
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/printing_visitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB_1 = $(BIN)/lab_1

all: $(LAB_1)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/printing_visitor$(OBJ_SUFF) : printing_visitor.h

$(LAB_1): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB1)
	./test.sh
