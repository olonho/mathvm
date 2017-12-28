ROOT   = .
VM_ROOT = ../../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -Wno-unused -Wno-c++0x-compat -std=c++11

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/local_variable_storage$(OBJ_SUFF) \
   $(OBJ)/code_interpreter$(OBJ_SUFF) \
   $(OBJ)/ast_to_bytecode$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF)\
   $(OBJ)/main$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

# INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
