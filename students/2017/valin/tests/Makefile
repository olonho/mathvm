VM_ROOT=../../../..
ROOT=.

include $(VM_ROOT)/common.mk

CFLAGS += -std=c++11

INCLUDE += -I../include

TESTS = test_pretty_printer

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/../pretty_printer$(OBJ_SUFF)

all: $(TESTS)

$(TESTS): %: $(OBJ)/%.o $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $< $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)

