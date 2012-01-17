ROOT   = .
VM_ROOT = ../../../..
#USER_DEFS = -j8
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/mytranslator$(OBJ_SUFF) $(OBJ)/myinterpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB_3 = $(BIN)/lab_3

all: $(LAB_3)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/translator$(OBJ_SUFF) : mytranslator.h

$(LAB_3): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 
