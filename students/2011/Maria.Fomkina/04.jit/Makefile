ROOT   = .
VM_ROOT = ../../../..
USER_OBJ = $(OBJ)/my_jit$(OBJ_SUFF) $(OBJ)/main$(OBJ_SUFF) $(OBJ)/my_translator$(OBJ_SUFF) $(OBJ)/bytecode_visitor$(OBJ_SUFF) $(OBJ)/mvm_code$(OBJ_SUFF)
DEPENDENCIES = 
USER_CFLAGS = -O0

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

#$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(DEPENDENCIES)
#	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB)
$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
