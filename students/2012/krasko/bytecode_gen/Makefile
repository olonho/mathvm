ROOT   = .
VM_ROOT = ../../../..
NO_JIT = 1

BYTECODE = $(BIN)/bytecode_gen

build: $(BYTECODE)

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) 

include $(VM_ROOT)/common.mk



$(BYTECODE): $(OUT) $(USER_OBJ) $(MATHVM_OBJ) 
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
