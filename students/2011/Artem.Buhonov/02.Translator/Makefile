ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/AstToBytecode$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB1 = $(BIN)/lab2

all: $(LAB2)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/AstToBytecode$(OBJ_SUFF) : AstToBytecode.h

$(LAB1): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB1)
	./test.sh
