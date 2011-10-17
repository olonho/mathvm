VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
#-DNDEBUG

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/main

DEPS = $(MATHVM_OBJ) parse.h InfoVisitor.h $(OBJ)/InfoVisitor.o  CodeVisitor.h  $(OBJ)/CodeVisitor.o MyCode.h $(OBJ)/MyCode.o MyTranslator.h

$(BIN)/main: dirs $(DEPS) $(OBJ)/main.o
	$(CXX) $(USER_FLAGS) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/main.o $(OBJ)/CodeVisitor.o $(OBJ)/MyCode.o $(OBJ)/InfoVisitor.o

dirs:
	mkdir -p ./build/debug
	mkdir -p  ./build/opt

