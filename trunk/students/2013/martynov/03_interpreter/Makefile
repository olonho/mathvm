ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/BCTranslator$(OBJ_SUFF) \
	$(OBJ)/BCInterpreter$(OBJ_SUFF)
					 
include $(VM_ROOT)/common.mk

CODE2BIN = $(BIN)/code2bin

all: $(CODE2BIN)

$(CODE2BIN): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)