ROOT   = .
VM_ROOT = ../../../..

NO_JIT      = 1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
           $(OBJ)/my_visitor$(OBJ_SUFF)


include $(VM_ROOT)/common.mk

SOURCE_PRINTER = $(BIN)/print_source_code

all: $(SOURCE_PRINTER)

$(SOURCE_PRINTER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

