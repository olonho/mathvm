ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/AstToBytecode$(OBJ_SUFF) $(OBJ)/Exceptions$(OBJ_SUFF) $(OBJ)/ExecutableCode$(OBJ_SUFF) $(OBJ)/FreeVarsFunction$(OBJ_SUFF) $(OBJ)/NewByteCode$(OBJ_SUFF) $(OBJ)/VarsSearcherVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB3 = $(BIN)/lab3

all: $(LAB3)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/AstToBytecode$(OBJ_SUFF) $(OBJ)/Exceptions$(OBJ_SUFF) $(OBJ)/ExecutableCode$(OBJ_SUFF) $(OBJ)/FreeVarsFunction$(OBJ_SUFF) $(OBJ)/NewByteCode$(OBJ_SUFF) $(OBJ)/VarsSearcherVisitor$(OBJ_SUFF) : AstToBytecode.h Exceptions.h ExecutableCode.h FreeVarsFunction.h NewByteCode.h VarsSearcherVisitor.h

$(LAB3): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB3)
	./test.sh
