ROOT   = .
VM_ROOT = ../../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -Wno-error=unused -g -std=c++11

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/bytecode_executor$(OBJ_SUFF) \
   $(OBJ)/execution_storage$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator_overrides$(OBJ_SUFF) \
   $(OBJ)/ast_bytecode_generator$(OBJ_SUFF) \
   $(OBJ)/context_storage$(OBJ_SUFF) \



include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include
INCLUDE += -I$(ROOT)/../2/

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
