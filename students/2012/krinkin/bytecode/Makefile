ROOT        = .
VM_ROOT     = ../../../..
USER_CFLAGS = 
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/typechecker$(OBJ_SUFF) \
        $(OBJ)/codegen$(OBJ_SUFF) \
        $(OBJ)/asttranslator$(OBJ_SUFF)

# i don't need AsmJit
NO_JIT      = 1

include $(VM_ROOT)/common.mk

ASTCHECKER = $(BIN)/astchecker

all: $(ASTCHECKER)

$(ASTCHECKER): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
