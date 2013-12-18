ROOT   = .
VM_ROOT = ../../../../

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/ast_printer$(OBJ_SUFF)
	
AST_PRINTER = $(BIN)/ast_printer

include $(VM_ROOT)/common.mk

all: $(AST_PRINTER)

$(AST_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
