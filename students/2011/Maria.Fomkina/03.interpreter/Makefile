ROOT   = .
VM_ROOT = ../../../..
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecode_visitor$(OBJ_SUFF) $(OBJ)/mvm_code$(OBJ_SUFF)
DEPENDENCIES = $(ROOT)/my_translator.h

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(DEPENDENCIES)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB)
