ROOT = .
VM_ROOT = ../../../..

include $(VM_ROOT)/common.mk

LAB2 = $(BIN)/lab2

all: $(LAB2)

$(OBJ)/main$(OBJ_SUFF): translator.hpp

$(LAB2): $(OUT) $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 
	$(CXX) -o $@ $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 

test: $(LAB2)
	./test_all.sh
