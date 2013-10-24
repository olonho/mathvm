ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/AstToCodePrinter$(OBJ_SUFF)
					 
include $(VM_ROOT)/common.mk

AST2CODE = $(BIN)/ast2code

all: $(AST2CODE)

$(AST2CODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
