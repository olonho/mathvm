ROOT   = .
VM_ROOT = ../../../..

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/bytecodetranslatorimpl$(OBJ_SUFF)\
   $(OBJ)/interpretercodeimpl$(OBJ_SUFF)\
   $(OBJ)/vmstack$(OBJ_SUFF)\
   $(OBJ)/abstractvarcontext$(OBJ_SUFF)\
   $(OBJ)/ptrmap$(OBJ_SUFF)\
   $(OBJ)/vmscopevars$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

LIBS += -lrt    

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
