ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/AstPrinter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

AST_PRINTER = $(BIN)/AstPrinter

all: $(AST_PRINTER)

$(AST_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
