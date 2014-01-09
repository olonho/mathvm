ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/BCTranslator$(OBJ_SUFF)
					 
include $(VM_ROOT)/common.mk

SCODE2BCODE = $(BIN)/scode2bcode

all: $(SCODE2BCODE)

$(SCODE2BCODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
