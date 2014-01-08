ROOT   = .
VM_ROOT = ../../../../

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/interpreter_impl$(OBJ_SUFF) \
	$(OBJ)/code_impl$(OBJ_SUFF) \
	$(OBJ)/translator_impl$(OBJ_SUFF)

	TRANSLATOR = $(BIN)/translator

include $(VM_ROOT)/common.mk

all: $(TRANSLATOR)

$(TRANSLATOR): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
