ROOT   = .
VM_ROOT = ../../../..
TEST_ROOT = $(VM_ROOT)/tests

# This flag disables JIT dependencies.
NO_JIT = 1

USER_CFLAGS = -std=c++11 -g

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF)\
   $(OBJ)/code_generator$(OBJ_SUFF)\
   $(OBJ)/generator_context$(OBJ_SUFF)\
   $(OBJ)/interpreter_context$(OBJ_SUFF)\
   $(OBJ)/code_interpreter$(OBJ_SUFF)\

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
