ROOT = .
VM_ROOT = ../../../..

include $(VM_ROOT)/common.mk

LAB1 = $(BIN)/lab1

all: $(LAB1)

$(OBJ)/main$(OBJ_SUFF): printer.hpp

$(LAB1): $(OUT) $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 
	$(CXX) -g3 -o $@ $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) 

test: $(LAB1)
	./test.sh
