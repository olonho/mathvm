VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
#-DNDEBUG
OPT = y
include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/main

DEPS = $(MATHVM_OBJ) $(OBJ)/main.o Interpreter.h $(OBJ)/Interpreter.o TranslationUtils.h $(OBJ)/TranslationUtils.o SymbolVisitor.h $(OBJ)/SymbolVisitor.o TypeVisitor.h $(OBJ)/TypeVisitor.o CodeVisitor.h  $(OBJ)/CodeVisitor.o Executable.h $(OBJ)/Executable.o Translator.h SymbolStack.h TranslationException.h parse.h

$(BIN)/main: dirs $(DEPS) $(OBJ)/main.o
	$(CXX) $(USER_FLAGS) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/main.o $(OBJ)/Interpreter.o $(OBJ)/TranslationUtils.o $(OBJ)/SymbolVisitor.o $(OBJ)/TypeVisitor.o $(OBJ)/CodeVisitor.o $(OBJ)/Executable.o
	
dirs:
	mkdir -p ./build/debug
	mkdir -p  ./build/opt

