ROOT   = .
VM_ROOT = ../../../..
USER_OBJ = $(OBJ)/TranslatorVisitor$(OBJ_SUFF) $(OBJ)/Scopes$(OBJ_SUFF) \
$(OBJ)/VariableScopeManager$(OBJ_SUFF) $(OBJ)/MyInterpreter$(OBJ_SUFF) $(OBJ)/MyTranslator$(OBJ_SUFF) $(OBJ)/main$(OBJ_SUFF)

USER_HEADERS = $(ROOT)/VarId.h $(ROOT)/FunctionId.h $(ROOT)/StackVariable.h $(ROOT)/StackFrame.h \
$(ROOT)/TranslationException.h $(ROOT)/InterpretationException.h $(ROOT)/ExtendedBytecodeFunction.h \
$(ROOT)/TranslatorVisitor.h  $(ROOT)/Scopes.h $(ROOT)/VariableScopeManager.h $(ROOT)/MyInterpreter.h $(ROOT)/MyTranslator.h $(ROOT)

include $(VM_ROOT)/common.mk

LAB3 = $(BIN)/lab3

all: $(LAB3)

$(LAB3): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

