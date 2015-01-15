ROOT = .
VM_ROOT = ../../../..

OPT=1
WITH_SDL=1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
                   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
                   $(OBJ)/bytecode_interpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MVM = $(BIN)/mvm

all: $(MVM)

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

