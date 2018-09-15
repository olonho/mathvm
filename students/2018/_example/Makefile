ROOT   = .
VM_ROOT = ../../..

# This flag enables -O2 optimization. Usage: make OPT=1
#OPT = 1

# This flag disables JIT dependencies (should be 0 for task #4).
NO_JIT = 1
# This flag enables SDL dependencies (should be 1 for task #4).
#WITH_SDL = 1

USER_OBJ = \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/translator_impl$(OBJ_SUFF)

USER_CFLAGS = -std=c++14

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

ifeq ($(OPT), 1)
KIND = opt
else
KIND = debug
endif

test: $(MATHVM)
	$(VM_ROOT)/tests/task1.py -k $(KIND) -t $(VM_ROOT)/tests/mark3/
	$(VM_ROOT)/tests/task1.py -k $(KIND) -t $(VM_ROOT)/tests/mark4/
	$(VM_ROOT)/tests/task1.py -k $(KIND) -t $(VM_ROOT)/tests/mark5/
	#$(VM_ROOT)/tests/task3.py -k $(KIND) -t $(VM_ROOT)/tests/mark3/
	#$(VM_ROOT)/tests/task3.py -k $(KIND) -t $(VM_ROOT)/tests/mark4/
	#$(VM_ROOT)/tests/task3.py -k $(KIND) -t $(VM_ROOT)/tests/mark5/

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
