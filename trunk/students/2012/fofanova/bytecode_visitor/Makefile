ROOT        = .
VM_ROOT     = ../../../..
USER_CFLAGS = 
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/bytecode_visitor$(OBJ_SUFF)

NO_JIT      = 1

include $(VM_ROOT)/common.mk

BYTECODEVISITOR = $(BIN)/bytecode_visitor

all: $(BYTECODEVISITOR)

$(BYTECODEVISITOR): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
