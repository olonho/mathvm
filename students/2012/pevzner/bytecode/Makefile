ROOT   = .
VM_ROOT = ../../../..
NO_JIT = 1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
		$(OBJ)/ExecutableCode$(OBJ_SUFF) \
		$(OBJ)/ASTtoByteCodeTranslator$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

BYTECODE = $(BIN)/bytecode

build: $(BYTECODE)

$(BYTECODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
