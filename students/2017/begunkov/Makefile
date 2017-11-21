ROOT   = .
VM_ROOT = ../../..

#OPT = 0
NO_JIT = 0
WITH_SDL = 1

USER_CFLAGS = -std=c++11 -Wno-unused-function -Wno-error=switch -Wno-switch -ldl

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/source_translator_impl$(OBJ_SUFF) \
   $(OBJ)/code_impl$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator_impl$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
