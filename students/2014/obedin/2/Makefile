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
   $(OBJ)/my_translator$(OBJ_SUFF)

USER_CFLAGS = -Wno-unused

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(OBJ)/my_translator$(OBJ_SUFF): my_translator.cpp my_translator-helpers.cpp my_translator-visitors.cpp

SCRIPT=

run:
	$(BIN)/mvm $(SCRIPT)
