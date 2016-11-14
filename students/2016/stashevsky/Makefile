ROOT   = .
VM_ROOT = ../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -std=c++11 -g
NO_JIT = 1

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/printer_translator$(OBJ_SUFF) \
   $(OBJ)/interpreter_code$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

debug: all
	gdb -tui --args $(BIN)/mvm -p $(VM_ROOT)/tests/astprint/dummyTest.mvm

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
