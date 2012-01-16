ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecoder$(OBJ_SUFF) $(OBJ)/my_code$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB3 = $(BIN)/lab3

all: $(LAB3)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecoder$(OBJ_SUFF)  $(OBJ)/bytecoder$(OBJ_SUFF): my_translator.h  bytecoder.h my_code.h

$(LAB3): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB3)
	./test.sh
