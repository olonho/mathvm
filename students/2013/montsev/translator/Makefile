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
   $(OBJ)/Translator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

CXX = g++

.PHONY: all
all: $(MATHVM)

.PHONY: rebuild
rebuild: 
	make clean && make

.PHONY: test
test: 
	make && ./run.py

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
