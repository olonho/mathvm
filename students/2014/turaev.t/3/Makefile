ROOT   = .
VM_ROOT = ../../../../

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11 -O3 -Wall -Wno-unused

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/BytecodeTranslator$(OBJ_SUFF) \
   $(OBJ)/BytecodeVisitor$(OBJ_SUFF) \
   $(OBJ)/Context$(OBJ_SUFF) \
   $(OBJ)/SimpleInterpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

run:
	./run_tests.sh