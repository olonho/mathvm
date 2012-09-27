ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/Ast2SrcVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

AST2SRC = $(BIN)/ast2src

all: $(AST2SRC)

$(AST2SRC): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(OBJ)/main$(OBJ_SUFF): $(OBJ)/Ast2SrcVisitor$(OBJ_SUFF)

$(OBJ)/Ast2SrcVisitor$(OBJ_SUFF): Ast2SrcVisitor.h Ast2SrcVisitor.cpp