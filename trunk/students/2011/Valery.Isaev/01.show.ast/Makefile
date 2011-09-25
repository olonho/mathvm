VM_ROOT = ../../../..
ROOT = .

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/show $(BIN)/tests

COMMON = $(MATHVM_OBJ) $(OBJ)/ShowVisitor.o
COMMON_BLD = $(COMMON) $(THREAD_LIB)
COMMON_TRG = $(COMMON) parse.cpp
COMMON_TRG = $(COMMON) parse.cpp ShowVisitor.h

$(BIN)/show: $(OBJ)/main.o $(COMMON_TRG)
	$(CXX) -o $@ $< $(COMMON_BLD)

$(BIN)/tests: $(OBJ)/tests.o $(COMMON_TRG) CompareVisitor.h $(OBJ)/CompareVisitor.o
	$(CXX) -o $@ $< $(COMMON_BLD) $(OBJ)/CompareVisitor.o
