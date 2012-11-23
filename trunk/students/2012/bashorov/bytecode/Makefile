ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/BytecodeTranslatorImpl$(OBJ_SUFF) \
	$(OBJ)/Runner$(OBJ_SUFF) \
	$(OBJ)/CodeImpl$(OBJ_SUFF) \
	$(OBJ)/Ast2BytecodeVisitor$(OBJ_SUFF)

NO_JIT = 1

include $(VM_ROOT)/common.mk

INTERPRETER = $(BIN)/interpreter

all: $(INTERPRETER) 
	./test.sh "$(INTERPRETER) -d" ./tests/ ./tests/ bytecode coloroff showall
	./test.sh "$(INTERPRETER)" ./tests/ ./tests/ interpreter coloroff showall

$(INTERPRETER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)