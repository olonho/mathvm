VM_ROOT = ../../../..
ROOT = .

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/gen_bcode

DEPS = $(MATHVM_OBJ) parse.h BCodeVisitor.h $(OBJ)/BCodeVisitor.o

$(BIN)/gen_bcode: $(DEPS) $(OBJ)/gen_bcode.o
	$(CXX) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/gen_bcode.o $(OBJ)/BCodeVisitor.o

