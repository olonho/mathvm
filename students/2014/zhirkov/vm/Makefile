ROOT   = .
VM_ROOT = ../../../..
NO_JIT = 1
ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11  
USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/ast_printer$(OBJ_SUFF)\
   $(OBJ)/ir/ir$(OBJ_SUFF)\
   $(OBJ)/ir/ir_printer$(OBJ_SUFF)\
   $(OBJ)/ir/transformations/identity$(OBJ_SUFF)\
   $(OBJ)/ir/transformations/substitutions$(OBJ_SUFF)\
   $(OBJ)/ir/transformations/ssa$(OBJ_SUFF)\
   $(OBJ)/ir/transformations/phi_values$(OBJ_SUFF)\
   $(OBJ)/ir/transformations/typecasts$(OBJ_SUFF)\
   $(OBJ)/ir/util$(OBJ_SUFF)\
   $(OBJ)/translator/closure_analyzer$(OBJ_SUFF)\
   $(OBJ)/translator/ssa_utils$(OBJ_SUFF)\
   $(OBJ)/translator/translator$(OBJ_SUFF)\
   $(OBJ)/util$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
