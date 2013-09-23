ROOT = .
VM_ROOT = ./../../../..
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
					 $(OBJ)/Converter$(OBJ_SUFF)
					 
include $(VM_ROOT)/common.mk

ASTTOCODE = $(BIN)/asttocode

all: $(ASTTOCODE)

$(ASTTOCODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
