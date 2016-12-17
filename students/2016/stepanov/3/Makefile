ROOT   = .
VM_ROOT = ../../../..
# This flag disables JIT dependencies.
NO_JIT = 0
# JIT_FLAGS = -DASMJIT_RELEASE
WITH_SDL = true


USER_CFLAGS = -std=c++11 -O2 -lstdc++ -Wl,--no-as-needed -DASMJIT_RECHECK_STATE -rdynamic -ldl -g $(JIT_FLAGS) 

ifneq ($(NO_JIT), 1)
# JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
JIT_OBJ =  $(OBJ)/JTranslator$(OBJ_SUFF) \
                   $(OBJ)/JScopeData$(OBJ_SUFF) \
                   $(OBJ)/JVariableAnnotator$(OBJ_SUFF) \
                   $(OBJ)/AScopeData$(OBJ_SUFF) \
                   $(OBJ)/jRuntime$(OBJ_SUFF)
else
JIT_OBJ =
endif


USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/BytecodeRFTranslator$(OBJ_SUFF)  \
   $(OBJ)/BytecodeRFInterpreter$(OBJ_SUFF)  \
   $(OBJ)/EmptyInterpreter$(OBJ_SUFF)  \
   $(OBJ)/StackItem$(OBJ_SUFF)  \
   $(OBJ)/InterScope$(OBJ_SUFF)  \
   $(OBJ)/ScopeData$(OBJ_SUFF)  \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator$(OBJ_SUFF) \
   $(OBJ)/RFUtils$(OBJ_SUFF)  \
   $(OBJ)/translator_create$(OBJ_SUFF)


include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
