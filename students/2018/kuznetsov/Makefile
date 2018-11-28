ROOT   = .
VM_ROOT = ../../..

# This flag enables -O2 optimization. Usage: make OPT=1
#OPT = 1

# This flag disables JIT dependencies (should be 0 for task #4).
NO_JIT = 1
# This flag enables SDL dependencies (should be 1 for task #4).
#WITH_SDL = 1

USER_OBJ = \
	$(OBJ)/astprinter/ast_printer$(OBJ_SUFF) \
	$(OBJ)/bytecode/program_state$(OBJ_SUFF) \
	$(OBJ)/bytecode/bytecode_translator$(OBJ_SUFF) \
	$(OBJ)/bytecode/bytecode_interpreter$(OBJ_SUFF) \
	$(OBJ)/main$(OBJ_SUFF) \
	$(OBJ)/translator_impl$(OBJ_SUFF)

USER_CFLAGS = -std=c++14

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: customdirs | $(MATHVM)

customdirs:
	mkdir -p $(OBJ)/astprinter
	mkdir -p $(OBJ)/bytecode

ifeq ($(OPT), 1)
KIND = opt
else
KIND = debug
endif

test: $(MATHVM)
	#python $(VM_ROOT)/tests/task1.py -k $(KIND) -t $(VM_ROOT)/tests/mark3/
	#python $(VM_ROOT)/tests/task1.py -k $(KIND) -t $(VM_ROOT)/tests/mark4/
	#python $(VM_ROOT)/tests/task1.py -k $(KIND) -t $(VM_ROOT)/tests/mark5/
	python $(VM_ROOT)/tests/task3.py -k $(KIND) -t $(VM_ROOT)/tests/mark3/
	python $(VM_ROOT)/tests/task3.py -k $(KIND) -t $(VM_ROOT)/tests/mark4/
	python $(VM_ROOT)/tests/task3.py -k $(KIND) -t $(VM_ROOT)/tests/mark5/

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)