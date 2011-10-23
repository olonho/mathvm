ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = 
USER_OBJ = $(OBJ)/main.01$(OBJ_SUFF) $(OBJ)/VisitorSourcePrinter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB0 = $(BIN)/lab0

all: $(LAB0)

$(OBJ)/main.01$(OBJ_SUFF) $(OBJ)/VisitorSourcePrinter$(OBJ_SUFF) : VisitorSourcePrinter.h

$(LAB0): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB1)
	./test.sh
