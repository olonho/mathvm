VM_ROOT = ../../../..
ROOT = .

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/translator

COMMON = $(MATHVM_OBJ) $(OBJ)/Translator.o $(OBJ)/AstShowVisitor.o
COMMON_BLD = $(COMMON) $(THREAD_LIB)
COMMON_TRG = $(COMMON) Exception.cpp
COMMON_TRG = $(COMMON) Exception.cpp Translator.h

$(BIN)/translator: $(OBJ)/main.o $(COMMON_TRG)
	$(CXX) -o $@ $< $(COMMON_BLD)
