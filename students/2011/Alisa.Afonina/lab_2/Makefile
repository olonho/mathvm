ROOT   = .
VM_ROOT = ../../../..
#USER_DEFS = -j8
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/translator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB_2 = $(BIN)/lab_2

all: $(LAB_2)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/translator$(OBJ_SUFF) : translator.h

$(LAB_2): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 
