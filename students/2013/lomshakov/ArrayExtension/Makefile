ROOT   = ./ArrayExtension
VM_ROOT = .
VM_ROOT_PARENT = ./../../../..

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/InterpreterImpl$(OBJ_SUFF) \
   $(OBJ)/BytecodeEmitter$(OBJ_SUFF)\
   $(OBJ)/Translator$(OBJ_SUFF)\
   $(OBJ)/BCTranslatorImpl$(OBJ_SUFF)\
   $(OBJ)/MSMemoryManager$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
