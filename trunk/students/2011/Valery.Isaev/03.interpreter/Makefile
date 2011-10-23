VM_ROOT = ../../../..
ROOT = .

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/interpreter

COMMON = $(MATHVM_OBJ) $(OBJ)/Translator.o $(OBJ)/ShowVisitor.o $(OBJ)/Interpreter.o

$(BIN)/interpreter: $(OBJ)/main.o $(COMMON) parse.cpp Translator.h
	$(CXX) -o $@ $< $(COMMON) $(THREAD_LIB)
