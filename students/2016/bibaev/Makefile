ROOT   = .
VM_ROOT = ../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -std=c++1y

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/printer$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

test: $(MATHVM) $(TEST_ROOT)
	($(MATHVM) tests/test.txt $(BIN)/res1.txt && $(MATHVM) $(BIN)/res1.txt $(BIN)/res2.txt && (diff $(BIN)/res1.txt $(BIN)/res2.txt && echo "Success")) || echo "Fail"
	
$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)
