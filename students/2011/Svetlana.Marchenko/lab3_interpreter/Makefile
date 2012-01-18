ROOT   = .
VM_ROOT = ../../../..
#USER_DEFS = -j8
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/TranslateVisitor$(OBJ_SUFF) $(OBJ)/VarTable$(OBJ_SUFF) $(OBJ)/MyCode$(OBJ_SUFF) $(OBJ)/BytecodeInterpreter$(OBJ_SUFF) $(OBJ)/DataStack$(OBJ_SUFF) $(OBJ)/FuncStackFrame$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB2 = $(BIN)/lab3_interpreter

all: $(LAB3)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/TranslateVisitor$(OBJ_SUFF) $(OBJ$(OBJ)/VarTable$(OBJ_SUFF) $(OBJ)/MyCode$(OBJ_SUFF) $(OBJ)/BytecodeInterpreter$(OBJ_SUFF) $(OBJ)/DataStack$(OBJ_SUFF) $(OBJ)/FuncStackFrame$(OBJ_SUFF)): TranslateVisitor.h VarTable.h  MyCode.h BytecodeInterpreter.h DataStack.h FuncStackFrame.h

$(LAB3): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 
