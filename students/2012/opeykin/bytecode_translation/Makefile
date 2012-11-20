ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/interpreter_code_impl$(OBJ_SUFF) \
	$(OBJ)/CodeBuilderVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

BYTECODE_TRANSLATION = $(BIN)/bytecode_translation

build:
all: $(BYTECODE_TRANSLATION)

$(BYTECODE_TRANSLATION): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
