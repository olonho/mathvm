ROOT   = .
VM_ROOT = ../../../..
# This flag disables JIT dependencies.
NO_JIT = 0
OPT = 1
WITH_SDL = 1

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/bc_translator$(OBJ_SUFF) \
   $(OBJ)/bc_gen$(OBJ_SUFF) \
   $(OBJ)/bc_interpreter$(OBJ_SUFF) \
   $(OBJ)/bc_jit$(OBJ_SUFF) \
   $(OBJ)/preprocessor$(OBJ_SUFF)

USER_CFLAGS = -std=c++11 -Wno-switch

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
