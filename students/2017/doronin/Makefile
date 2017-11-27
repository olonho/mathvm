ROOT   = .
VM_ROOT = ../../..

OPT = 1
NO_JIT = 1
WITH_SDL = 1

USER_CFLAGS = -std=c++11 -O3

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/printer_translator_impl$(OBJ_SUFF) \
   $(OBJ)/q_bytecode_translator_impl$(OBJ_SUFF) \
   $(OBJ)/code_impl$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -c native_call.s -o $(OBJ)/native_call$(OBJ_SUFF) 
	$(CXX) -o $@ $(USER_OBJ) $(OBJ)/native_call$(OBJ_SUFF) $(MATHVM_OBJ) $(LIBS)
