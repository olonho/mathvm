ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3 -std=c++0x
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/PresentationVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB0 = $(BIN)/lab0

all: $(LAB0)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/PresentationVisitor$(OBJ_SUFF) : PresentationVisitor.h

$(LAB0): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB1)
	./test.sh
