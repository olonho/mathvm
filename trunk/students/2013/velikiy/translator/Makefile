ROOT   = .
VM_ROOT = ../../../..
USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_imp$(OBJ_SUFF) \
   $(OBJ)/translatorbytecode_imp$(OBJ_SUFF) 
   
include $(VM_ROOT)/common.mk

COMPLILE_LIB = $(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) 

$(OBJ)/translator_imp$(OBJ_SUFF): imp/translator.cpp
	$(COMPLILE_LIB) imp/translator.cpp -o $(OBJ)/translator_imp$(OBJ_SUFF)

$(OBJ)/translatorbytecode_imp$(OBJ_SUFF): imp/BytecodeTranslatorImpl.cpp
	$(COMPLILE_LIB) imp/BytecodeTranslatorImpl.cpp $(OBJ)/translator_imp$(OBJ_SUFF) -o $(OBJ)/translatorbytecode_imp$(OBJ_SUFF)

APP = $(BIN)/app

all: $(APP)

$(APP): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

