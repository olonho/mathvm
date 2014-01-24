ROOT   = .
VM_ROOT = ./../../../..

OPT = 1 
WITH_SDL = false
NO_JIT = 1
# USER_LIBS = -lavcall

USER_OBJ = \
	$(OBJ)/cinterpreter$(OBJ_SUFF) \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/Translator$(OBJ_SUFF) \
	$(OBJ)/AbstractInterpreter$(OBJ_SUFF) \
	$(OBJ)/helpers$(OBJ_SUFF) \
	$(OBJ)/AbstractContext$(OBJ_SUFF) \
	$(OBJ)/VisitorEater$(OBJ_SUFF) \
	$(OBJ)/Visitors$(OBJ_SUFF) \
	$(OBJ)/constants_pool$(OBJ_SUFF) \
	$(OBJ)/context$(OBJ_SUFF) \
	$(OBJ)/function$(OBJ_SUFF) \
	$(OBJ)/diagnostics$(OBJ_SUFF) \
	$(OBJ)/Annotation$(OBJ_SUFF) \
	$(OBJ)/TypeInferrer$(OBJ_SUFF) \
	$(OBJ)/StackSizeInferrer$(OBJ_SUFF) \
	$(OBJ)/hashtable$(OBJ_SUFF) 

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) 
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)


