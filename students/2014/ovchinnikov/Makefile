CXX = g++
ROOT   = .
VM_ROOT = ../../..
WITH_SDL = true

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11 -Wno-error -Wall -Wno-unused

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translatorfactory$(OBJ_SUFF) \
   $(OBJ)/astprinter$(OBJ_SUFF) \
   $(OBJ)/bytecodetranslator$(OBJ_SUFF) \
   $(OBJ)/bytecodeinterpreter$(OBJ_SUFF) \
   $(OBJ)/blockscope$(OBJ_SUFF)


include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

