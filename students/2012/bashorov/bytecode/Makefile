ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/BytecodeTranslatorImpl$(OBJ_SUFF) \
	$(OBJ)/CodeImpl$(OBJ_SUFF) \
	$(OBJ)/Ast2BytecodeVisitor$(OBJ_SUFF)

NO_JIT = 1

include $(VM_ROOT)/common.mk

TRANSLATOR = $(BIN)/translator

all: $(TRANSLATOR) 
	./test.sh "$(TRANSLATOR)" ./tests/ ./tests/ coloroff showall

$(TRANSLATOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)