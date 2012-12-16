ROOT        = .
VM_ROOT     = ../../../..
USER_CFLAGS = -Wno-write-strings
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/bytecode_visitor$(OBJ_SUFF) \
        $(OBJ)/interpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

JITVISITOR = $(BIN)/bytecode_visitor

all: $(JITVISITOR)

$(JITVISITOR): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
