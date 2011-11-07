VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
#-DNDEBUG
#OPT = y
include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/main

ASMJIT_OBJ += $(OBJ)/CompilerX86X64$(OBJ_SUFF) \
	         $(OBJ)/Compiler$(OBJ_SUFF) \
	         $(OBJ)/Operand$(OBJ_SUFF) \
	         $(OBJ)/Logger$(OBJ_SUFF) \
	         $(OBJ)/Assembler$(OBJ_SUFF)

DEPS = $(MATHVM_OBJ) $(ASMJIT_OBJ) $(OBJ)/main.o 

$(BIN)/main: dirs $(DEPS) $(OBJ)/main.o
	$(CXX) $(USER_FLAGS) -o $@ $(DEPS) $(THREAD_LIB)
	
dirs:
	mkdir -p ./build/debug
	mkdir -p  ./build/opt

