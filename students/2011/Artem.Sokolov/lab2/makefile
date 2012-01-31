ROOT = .
VM_ROOT = ../../../..

include $(VM_ROOT)/common.mk

LAB2 = $(BIN)/lab2

all: $(LAB2)

$(OBJ)/main$(OBJ_SUFF): translate.hpp mycode.hpp visitor.hpp

$(LAB2): $(OUT) $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 
	$(CXX) -g3 -o $@ $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 

test: $(LAB2)
	./test.sh
