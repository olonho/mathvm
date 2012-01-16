ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecoder$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB2 = $(BIN)/lab2

all: $(LAB2)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecoder$(OBJ_SUFF) : my_translator.h  bytecoder.h

$(LAB2): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB2)
	./test.sh
