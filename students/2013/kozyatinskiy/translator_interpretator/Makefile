ROOT   = .
VM_ROOT = ../../../../

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/MyBytecode$(OBJ_SUFF) \
	$(OBJ)/MyFuncScope$(OBJ_SUFF) \
	$(OBJ)/MyInstruction$(OBJ_SUFF) \
	$(OBJ)/MyInterpreter$(OBJ_SUFF) \
	$(OBJ)/MyMemoryManager$(OBJ_SUFF) \
	$(OBJ)/MyProgram$(OBJ_SUFF) \
	$(OBJ)/MyTypeStack$(OBJ_SUFF) \
	$(OBJ)/MyVarScope$(OBJ_SUFF) \
	$(OBJ)/PrintVisitor$(OBJ_SUFF) \
	$(OBJ)/StringStorage$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
