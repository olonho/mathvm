ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecoder$(OBJ_SUFF) $(OBJ)/my_code$(OBJ_SUFF)

include common.mk
#include $(VM_ROOT)/common.mk

LAB4 = $(BIN)/lab4

all: $(LAB4)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecoder$(OBJ_SUFF)  $(OBJ)/bytecoder$(OBJ_SUFF): my_translator.h  bytecoder.h my_code.h config.h

$(LAB4): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB4)
	./test.sh
