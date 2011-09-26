VM_ROOT = ../../../..
ROOT = .

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/show_src $(BIN)/tests

DEPS = $(MATHVM_OBJ) parse.h $(OBJ)/ShowVisitor.o

$(BIN)/show_src: $(DEPS) $(OBJ)/show_src.o
	$(CXX) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/show_src.o $(OBJ)/ShowVisitor.o

$(BIN)/tests: $(DEPS) $(OBJ)/tests.o
	$(CXX) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/show_src.o $(OBJ)/ShowVisitor.o
