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
   $(OBJ)/translator_create$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/typechecking$(OBJ_SUFF) \
   $(OBJ)/bytecode_generator$(OBJ_SUFF) \
   $(OBJ)/my_utils$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

USER_CFLAGS = -Wno-unused -Wno-c++0x-compat -std=c++11

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
