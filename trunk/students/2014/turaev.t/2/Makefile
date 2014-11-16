ROOT   = .
VM_ROOT = ../../../..

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11 -O3 -Wall -Wno-unused

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/BytecodeVisitor$(OBJ_SUFF) \
   $(OBJ)/Context$(OBJ_SUFF) \
   $(OBJ)/SimpleInterpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm
TRANSLATOR = $(BIN)/bcp

all: $(MATHVM) $(TRANSLATOR)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(OBJ)/main_interpreter$(OBJ_SUFF)
	$(CXX) -o $@ $(OBJ)/main_interpreter$(OBJ_SUFF) $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(TRANSLATOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(OBJ)/main_translator$(OBJ_SUFF)
	$(CXX) -o $@ $(OBJ)/main_translator$(OBJ_SUFF) $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
