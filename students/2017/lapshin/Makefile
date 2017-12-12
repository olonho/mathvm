ROOT   = .
VM_ROOT = ../../..

#OPT = 0
NO_JIT = 0
WITH_SDL = 1

USER_CFLAGS = -std=c++17 -Wall -Wextra

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
	$(JIT_OBJ) \
	$(OBJ)/bytecode$(OBJ_SUFF) \
	$(OBJ)/bytecode_translator$(OBJ_SUFF) \
	$(OBJ)/code$(OBJ_SUFF) \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/printer$(OBJ_SUFF) \
	$(OBJ)/translator_impl$(OBJ_SUFF) \
	$(OBJ)/util$(OBJ_SUFF)

USER_DEPS += \
	$(ROOT)/Makefile \
	include/bytecode.h \
	include/bytecode_translator.h \
	include/code.h \
	include/printer.h \
	include/util.h

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
