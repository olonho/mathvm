ROOT        = .
VM_ROOT     = ../../../..
USER_CFLAGS = 
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/bcinterpreter$(OBJ_SUFF) \
        $(OBJ)/typer$(OBJ_SUFF) \
        $(OBJ)/generator$(OBJ_SUFF) \
        $(OBJ)/bccode$(OBJ_SUFF)

# i don't need AsmJit
NO_JIT      = 1

include $(VM_ROOT)/common.mk

INTERPRETER = $(BIN)/interpreter

all: $(INTERPRETER)

$(INTERPRETER): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
