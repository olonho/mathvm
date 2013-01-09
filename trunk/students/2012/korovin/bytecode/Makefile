ROOT   = .
VM_ROOT = ../../../..
NO_JIT = 0

USER_OBJ = $(OBJ)/BytecodeBuilder$(OBJ_SUFF) \
		$(OBJ)/BytecodeImpl$(OBJ_SUFF) \
		$(OBJ)/BytecodeVisitor$(OBJ_SUFF) \
		$(OBJ)/main$(OBJ_SUFF) \
		#$(OBJ)/test$(OBJ_SUFF) \
		

include $(VM_ROOT)/common.mk

BYTECODE = $(BIN)/bytecode

default: $(BYTECODE)

$(BYTECODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
