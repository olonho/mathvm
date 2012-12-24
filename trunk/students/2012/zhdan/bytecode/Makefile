ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/astinfo$(OBJ_SUFF) \
	   $(OBJ)/bytecodegen$(OBJ_SUFF) \
	   $(OBJ)/typechecker$(OBJ_SUFF)

NO_JIT = 1

include $(VM_ROOT)/common.mk

TRANSLATOR = $(BIN)/bytecodegen

all: $(TRANSLATOR)

$(TRANSLATOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
