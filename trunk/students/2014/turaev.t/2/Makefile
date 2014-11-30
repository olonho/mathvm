ROOT   = .
VM_ROOT = ../../../..
WITH_SDL=1
OPT=1

include $(VM_ROOT)/common.mk

USER_CFLAGS += -std=c++11 -Wno-unused -march=native -mtune=native

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/BytecodeVisitor$(OBJ_SUFF) \
   $(OBJ)/Context$(OBJ_SUFF) \
   $(OBJ)/SimpleInterpreter$(OBJ_SUFF) \
   $(OBJ)/NativeFunctionGeneratorTemplates$(OBJ_SUFF) \
   $(OBJ)/NativeFunctionGeneratorAsmJit$(OBJ_SUFF)

MATHVM = $(BIN)/mvm
TRANSLATOR = $(BIN)/bcp

all: $(MATHVM) $(TRANSLATOR)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(OBJ)/main_interpreter$(OBJ_SUFF)
	$(CXX) -o $@ $(OBJ)/main_interpreter$(OBJ_SUFF) $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(TRANSLATOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(OBJ)/main_translator$(OBJ_SUFF)
	$(CXX) -o $@ $(OBJ)/main_translator$(OBJ_SUFF) $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
