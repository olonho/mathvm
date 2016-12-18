ROOT   = .
VM_ROOT = ../../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -std=c++11

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/my_translator$(OBJ_SUFF) \
   $(OBJ)/my_interpreter$(OBJ_SUFF) \
   $(OBJ)/context$(OBJ_SUFF) \
   $(OBJ)/instruction_util$(OBJ_SUFF) \
   $(OBJ)/stack$(OBJ_SUFF) \
   $(OBJ)/bytecode_stream$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) $(USER_CFLAGS) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
