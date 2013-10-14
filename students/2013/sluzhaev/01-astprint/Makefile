ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
    $(OBJ)/main$(OBJ_SUFF) \
    $(OBJ)/ast_print_visitor$(OBJ_SUFF)

NO_JIT = 1
ASTPRINT = $(BIN)/astprint

include $(VM_ROOT)/common.mk

all: $(ASTPRINT)

$(ASTPRINT): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
