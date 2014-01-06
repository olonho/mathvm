ROOT = .
VM_ROOT = ../../../..

USER_OBJ = \
    $(OBJ)/main$(OBJ_SUFF)\
    $(OBJ)/BytecodeTranslator$(OBJ_SUFF)

USER_DEFS = -DDEBUG

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
