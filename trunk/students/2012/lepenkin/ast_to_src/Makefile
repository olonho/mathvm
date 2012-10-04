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
   $(OBJ)/printerTest$(OBJ_SUFF) \
   $(OBJ)/PrinterVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

ASTPrinter = $(BIN)/printerTest

all: $(ASTPrinter)

$(ASTPrinter): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
