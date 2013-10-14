ROOT = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
		   $(OBJ)/interpreter_code$(OBJ_SUFF) \
		   $(OBJ)/bc_translator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MVM = $(BIN)/mvm

all: $(MVM)

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
