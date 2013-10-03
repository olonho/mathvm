ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/source_printer$(OBJ_SUFF)

NO_JIT      = 1

include $(VM_ROOT)/common.mk

SOURCE_PRINTER = $(BIN)/source_printer

all: $(SOURCE_PRINTER)

$(SOURCE_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

