VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
#-DNDEBUG

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/main

DEPS = $(MATHVM_OBJ) parse.h CodeVisitor.h $(OBJ)/CodeVisitor.o MyCode.h $(OBJ)/MyCode.o MyTranslator.h

$(BIN)/main: $(DEPS) $(OBJ)/main.o
	$(CXX) $(USER_FLAGS) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/main.o $(OBJ)/CodeVisitor.o $(OBJ)/MyCode.o

