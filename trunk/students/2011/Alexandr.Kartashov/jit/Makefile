VM_ROOT      = ../../../..
ROOT         = .
LOCAL_INC    = -I$(ROOT)/..
USER_LIBS    = -ldl
USER_DEFS    = -ggdb3 -DUSER_DATA_FIELD $(LOCAL_INC) -Wno-unused-function
USER_OBJS    =  $(OBJ)/mymain$(OBJ_SUFF)            \
		$(OBJ)/NativeTranslator$(OBJ_SUFF)  \
		$(OBJ)/Runtime$(OBJ_SUFF)           \
		$(OBJ)/FunctionCollector$(OBJ_SUFF)
USER_HEADERS = $(shell ls *.h)
RUN_TESTS    = tests/run.py
BASH         = $(shell which bash)

include $(VM_ROOT)/common.mk

MVM  = $(BIN)/mvm

all: .ast-patched $(MVM)

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJS) $(UTIL) $(USER_HEADERS)
	$(CXX) -o $@ $(MATHVM_OBJ) $(LOCAL_INC) $(USER_OBJS) $(THREAD_LIB) $(USER_LIBS)

tests:
	ln -s $(VM_ROOT)/tests .

test: tests $(MVM)
	$(RUN_TESTS)

rebuild:
	rm -f $(USER_OBJS)
	make