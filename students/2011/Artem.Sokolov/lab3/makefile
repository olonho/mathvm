ROOT = .
VM_ROOT = ../../../..

include $(VM_ROOT)/common.mk

LAB3 = $(BIN)/lab3

all: $(LAB3)

$(OBJ)/main$(OBJ_SUFF): translate.hpp mycode.hpp visitor.hpp

$(LAB3): $(OUT) $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 
	$(CXX) -g3 -o $@ $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 

test: $(LAB3)
	./test.sh
