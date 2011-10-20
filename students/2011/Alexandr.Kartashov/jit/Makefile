VM_ROOT      = ../../../..
ROOT         = .
LOCAL_INC    = -I$(ROOT)/..
USER_DEFS    = -ggdb3 -DUSER_DATA_FIELD $(LOCAL_INC)
USER_OBJS    = $(OBJ)/mymain$(OBJ_SUFF) $(OBJ)/NativeTranslator$(OBJ_SUFF) $(OBJ)/Runtime$(OBJ_SUFF)
USER_HEADERS = $(shell ls *.h)
RUN_TESTS    = tests/run.py
BASH         = $(shell which bash)

include $(VM_ROOT)/common.mk

MVM  = $(BIN)/mvm

all: .mathvm-patched .ast-patched $(MVM)

.ast-patched:
	patch -p0 -d $(VM_ROOT)/include < ast.patch
	touch .ast-patched

.mathvm-patched:
	patch -p0 -d $(VM_ROOT)/include < mathvm.patch
	touch .mathvm-patched

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJS) $(UTIL) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(LOCAL_INC) $(USER_OBJS) $(THREAD_LIB)

tests:
	ln -s $(VM_ROOT)/tests .

test: tests $(MVM)
	$(RUN_TESTS)

rebuild:
	rm -f $(USER_OBJS)
	make