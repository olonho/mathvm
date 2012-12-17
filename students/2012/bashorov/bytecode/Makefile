ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/BytecodeTranslatorImpl$(OBJ_SUFF) \
	$(OBJ)/Runner$(OBJ_SUFF) \
	$(OBJ)/CodeImpl$(OBJ_SUFF) \
	$(OBJ)/Ast2BytecodeVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INTERPRETER = $(BIN)/interpreter

$(INTERPRETER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

build_: $(INTERPRETER)

run: build_
	./test.sh "$(INTERPRETER) -d" ./tests/ ./tests/ bytecode coloroff showall
	./test.sh "$(INTERPRETER)" ./tests/ ./tests/ interpreter coloroff showall

all: build_
