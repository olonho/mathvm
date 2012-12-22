ROOT   = .
VM_ROOT = ../../../..

NO_JIT = 1

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/test$(OBJ_SUFF) \
   $(OBJ)/Translator$(OBJ_SUFF) \
   $(OBJ)/Interpreter$(OBJ_SUFF) \
   $(OBJ)/CodeImpl$(OBJ_SUFF)
   

include $(VM_ROOT)/common.mk

Translator = $(BIN)/test

all: $(Translator)

$(Translator): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
