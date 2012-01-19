VM_ROOT = ../../../..
ROOT = .

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/interpreter

COMMON = $(MATHVM_OBJ) $(OBJ)/Translator.o $(OBJ)/AstShowVisitor.o $(OBJ)/MvmCode.o

$(BIN)/interpreter: $(OBJ)/main.o $(COMMON) Translator.h BytecodeFunction.h MvmCode.h Exception.cpp Label.h
	$(CXX) -o $@ $< $(COMMON) $(THREAD_LIB)
