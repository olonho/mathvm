ROOT   = .
VM_ROOT = ../../../..
#USER_DEFS = -j8
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/ByteCodeGenerator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB1 = $(BIN)/lab1

all: $(LAB1)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/ByteCodeGenerator$(OBJ_SUFF) : ByteCodeGenerator.h

$(LAB1): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

