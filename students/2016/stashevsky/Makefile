ROOT   = .
VM_ROOT = ../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -g0 -std=c++11 -Ofast

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
   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/vm$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

debug: all
	gdb -tui --args $(BIN)/mvm $(VM_ROOT)/tests/optional/function_native.mvm
test:
	./tests/run.py
	./tests/run.py -t ./tests/additional/
	./tests/run.py -t ./tests/optional/
$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
