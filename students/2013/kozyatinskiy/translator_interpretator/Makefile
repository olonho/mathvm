ROOT   = .
VM_ROOT = ../../../../

#OPT = 1

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/CallStack$(OBJ_SUFF) \
	$(OBJ)/CompilerVisitor$(OBJ_SUFF) \
	$(OBJ)/Interpreter$(OBJ_SUFF) \
	$(OBJ)/StackLayout$(OBJ_SUFF) \
	$(OBJ)/StackVisitor$(OBJ_SUFF) \
	$(OBJ)/VarsUtil$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
