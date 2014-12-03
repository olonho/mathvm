ROOT   = .
VM_ROOT = ../../../..
OPT=1
ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
    $(JIT_OBJ) \
    $(OBJ)/main$(OBJ_SUFF) \
    $(OBJ)/bytecode_visitor$(OBJ_SUFF)

USER_CFLAGS = -Wno-unused

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)