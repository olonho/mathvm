ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/printer_visitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

PRINTER_VISITOR = $(BIN)/printer_visitor

all: $(PRINTER_VISITOR)

$(PRINTER_VISITOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
