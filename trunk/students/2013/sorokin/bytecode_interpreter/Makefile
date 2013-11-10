ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/TypeCalculator$(OBJ_SUFF) \
	$(OBJ)/VMRuntime$(OBJ_SUFF) \
	$(OBJ)/CodeTranslator$(OBJ_SUFF) \
	$(OBJ)/InterpreterCodeImpl$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

BYTECODE = $(BIN)/bytecode

build:
all: $(BYTECODE)

$(BYTECODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
