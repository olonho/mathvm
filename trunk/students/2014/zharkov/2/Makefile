OPT = true
ROOT   = .
VM_ROOT = ../../../../
WITH_SDL = true

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_CFLAGS += -std=c++11  

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/interpreter_impl$(OBJ_SUFF) \
   $(OBJ)/jit_builder$(OBJ_SUFF) \
   $(OBJ)/stack_machine$(OBJ_SUFF)

include $(VM_ROOT)/common.mk
-include Makefile.dep
Makefile.dep: $(ROOT)/*.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(VM_INCLUDE)  -MM  $(ROOT)/*.cpp | sed 's,^\(.*\$(OBJ_SUFF)\),$(OBJ)/\1,' > $@ 

MATHVM = $(BIN)/mvm

all: $(MATHVM)

tests: all 
	./tests/run.py -t translator $(OPT)
	./tests/run.py -e fail $(OPT)
	./tests/run.py -e interpreter $(OPT) 
	./tests/run.py -e given $(OPT)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
