ROOT   = .
VM_ROOT = ../../../..

NO_JIT = 1

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11 -w

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/bytecode_translator_impl$(OBJ_SUFF)\
   $(OBJ)/bytecode_visitor$(OBJ_SUFF)\
 $(OBJ)/interpreter_code_impl$(OBJ_SUFF)\
   $(OBJ)/main$(OBJ_SUFF)\


   
   


include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
