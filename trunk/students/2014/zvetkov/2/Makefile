OPT = 1
ROOT   = .
VM_ROOT = ../../../../

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/info$(OBJ_SUFF) \
   $(OBJ)/context$(OBJ_SUFF) \
   $(OBJ)/errors$(OBJ_SUFF) \
   $(OBJ)/translation_utils$(OBJ_SUFF) \
   $(OBJ)/bytecode_generator$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/bytecode_interpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)