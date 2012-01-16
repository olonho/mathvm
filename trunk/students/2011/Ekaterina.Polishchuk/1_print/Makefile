ROOT   = .
VM_ROOT = ../../../..
USER_OBJ = $(OBJ)/MyVisitor$(OBJ_SUFF) $(OBJ)/main$(OBJ_SUFF)
USER_HEADERS = $(ROOT)/MyVisitor.h 

include $(VM_ROOT)/common.mk

LAB1 = $(BIN)/lab1

all: $(LAB1)

$(LAB1): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 


