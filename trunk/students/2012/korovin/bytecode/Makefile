ROOT   = .
VM_ROOT = ../../../..
NO_JIT = 1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
		$(OBJ)/BytecodeBuilder$(OBJ_SUFF) \
		$(OBJ)/BytecodeImpl$(OBJ_SUFF) \
		$(OBJ)/BytecodeVisitor$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

BYTECODE = $(BIN)/bytecode

build: $(BYTECODE)

$(BYTECODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
