ROOT   = .
VM_ROOT = ../../../..

#OPT = 0
NO_JIT = 1
#WITH_SDL = 0

USER_CFLAGS = -std=c++11

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/my_vm/astprinter$(OBJ_SUFF) \
   $(OBJ)/my_vm/astprinter_visitor$(OBJ_SUFF) \
   $(OBJ)/my_vm/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/my_vm/bytecode_translator_visitor$(OBJ_SUFF) \
   $(OBJ)/my_vm/interpreter$(OBJ_SUFF) \
   $(OBJ)/my_vm/native_caller$(OBJ_SUFF) \
   $(OBJ)/my_vm/translator_factory$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/my_include

MATHVM = $(BIN)/mvm

MYVM = $(BIN)/my_vm
MKDIR = mkdir -p

all: $(MYVM) $(MATHVM)

$(MYVM):
	$(MKDIR) $(MYVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
