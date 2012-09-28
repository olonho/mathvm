ROOT   = .
VM_ROOT = ../../../..

NO_JIT = 1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/SourceByASTPrinter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

AST_PRINTER = $(BIN)/ast_printer

all: $(AST_PRINTER)

$(AST_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
