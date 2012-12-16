ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/astPrinter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk
CFLAGS += -march=native

AST_PRINTER = $(BIN)/ast_printer

all: $(AST_PRINTER)

$(AST_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(OBJ)/%$(OBJ_SUFF): astPrinter.h astPrinter.cpp main.cpp
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

