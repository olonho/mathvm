ROOT   = .
VM_ROOT = ../../../..

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/validating_visitor$(OBJ_SUFF) \
   $(OBJ)/ast_printer$(OBJ_SUFF) \
   $(OBJ)/bytecode_generator$(OBJ_SUFF) \
   $(OBJ)/my_translator$(OBJ_SUFF) \
   $(OBJ)/interpreter_impl$(OBJ_SUFF) \
   $(OBJ)/native_call_builder$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
