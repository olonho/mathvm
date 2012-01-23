ROOT   = .
VM_ROOT = ../../../..
#USER_DEFS = -j8
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/TranslateVisitor$(OBJ_SUFF) $(OBJ)/VarTable$(OBJ_SUFF) $(OBJ)/MyCode$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB2 = $(BIN)/lab2_translator

all: $(LAB2)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/TranslateVisitor$(OBJ_SUFF) $(OBJ)/VarTable$(OBJ_SUFF) $(OBJ)/MyCode$(OBJ_SUFF): TranslateVisitor.h VarTable.h  MyCode.h

$(LAB2): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 
