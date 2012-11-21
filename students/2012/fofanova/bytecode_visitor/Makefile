ROOT        = .
VM_ROOT     = ../../../..
USER_CFLAGS = -Wno-write-strings
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/bytecode_visitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

BYTECODEVISITOR = $(BIN)/bytecode_visitor $(BIN)/interpreter

all: $(BYTECODEVISITOR)

$(BYTECODEVISITOR): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
