ROOT   = .
VM_ROOT = ../../../..

all: build

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) 

include $(VM_ROOT)/common.mk

AST_PRINTER = $(BIN)/AstPrinter

build: $(AST_PRINTER)

$(AST_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
