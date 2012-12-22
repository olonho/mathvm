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

# for runing in Sublime
run: test

testAll: testBytecode testInterpreter

testBytecode: build_
	./test.sh "$(INTERPRETER) -d" ./tests/ ./tests/ bytecode coloroff showall

testInterpreter: build_
	./test.sh "$(INTERPRETER)" ./tests/ ./tests/ interpreter coloroff show

testInterpreterExt: build_
	./test.sh "$(INTERPRETER)" $(VM_ROOT)/tests/ $(VM_ROOT)/tests/ "" coloroon show

extTest:
	$(VM_ROOT)/tests/run.py -v -e $(INTERPRETER) -t $(VM_ROOT)/tests/

all: build_
