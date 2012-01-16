ROOT     = .
VM_ROOT  = ../../../..
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/MyTranslator$(OBJ_SUFF)
USER_HEADERS = $(ROOT)/TranslationException.h $(ROOT)/MyCode.h $(ROOT) $(ROOT)/VariableMap.h $(ROOT)

include $(VM_ROOT)/common.mk

LAB2 = $(BIN)/lab2

all: $(LAB2)

$(LAB2): $(OUT) $(MATHVM_OBJ) $(USER_OBJ) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 
