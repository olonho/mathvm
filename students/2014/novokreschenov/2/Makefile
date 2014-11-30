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
   $(OBJ)/bytecodetranslator$(OBJ_SUFF) \
   $(OBJ)/bytecodegenerator$(OBJ_SUFF) \
   $(OBJ)/contextmanager$(OBJ_SUFF) \
   $(OBJ)/functioncontext$(OBJ_SUFF) \
   $(OBJ)/interpretercode$(OBJ_SUFF) \
   $(OBJ)/typeinferencer$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
