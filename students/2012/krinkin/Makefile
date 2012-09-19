ROOT        = .
VM_ROOT     = ../../..
USER_CFLAGS = -std=c++0x
USER_OBJ    = \
        $(OBJ)/main$(OBJ_SUFF) \
        $(OBJ)/pretty$(OBJ_SUFF)

include $(ROOT)/common.mk

ASTPRINTER = $(BIN)/astprinter

all: $(ASTPRINTER)

$(ASTPRINTER): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ)
