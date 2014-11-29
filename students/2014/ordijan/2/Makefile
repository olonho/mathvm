ROOT   = .
VM_ROOT = ../../../..
OPT = true
#WITH_SDL = true

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11


USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/bytecode_interpreter$(OBJ_SUFF) \
   $(OBJ)/native_builder$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
