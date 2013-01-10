ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/bytecode_visitor$(OBJ_SUFF) \
   $(OBJ)/interprer$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

TRANSLATOR = $(BIN)/mvm

all: $(TRANSLATOR)

$(TRANSLATOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
