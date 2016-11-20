ROOT   = .
VM_ROOT = ../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = "-std=c++11"

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/PrinterTranslator$(OBJ_SUFF) \
   $(OBJ)/PrinterVisitor$(OBJ_SUFF) \
	 $(OBJ)/BytecodeCode$(OBJ_SUFF) \
	 $(OBJ)/BytecodeTranslator_impl$(OBJ_SUFF) \
	 $(OBJ)/BytecodeTranslatorVisitor$(OBJ_SUFF) \
	 $(OBJ)/TranslationContext$(OBJ_SUFF) \
	 $(OBJ)/TranslationException$(OBJ_SUFF) \
	 $(OBJ)/Interpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
