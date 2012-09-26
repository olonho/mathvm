ROOT        = .
VM_ROOT     = ../../..
USER_CFLAGS = 
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/pretty$(OBJ_SUFF)

# i don't need AsmJit
NO_JIT      = 1

include $(VM_ROOT)/common.mk

ASTPRINTER = $(BIN)/astprinter

all: $(ASTPRINTER)

$(ASTPRINTER): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
