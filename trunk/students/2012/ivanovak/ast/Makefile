ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/astprinter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

AST = $(BIN)/ast

all: $(AST)

$(AST): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
