ROOT = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/printer$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

PRINTER = $(BIN)/printer

all: $(PRINTER)

$(PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
