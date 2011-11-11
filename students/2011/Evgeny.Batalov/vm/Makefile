VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
#-DNDEBUG
#OPT = y
include $(VM_ROOT)/common.mk

ASMJIT_OBJ += $(OBJ)/CompilerX86X64$(OBJ_SUFF) \
	         $(OBJ)/Compiler$(OBJ_SUFF) \
	         $(OBJ)/Operand$(OBJ_SUFF) \
	         $(OBJ)/Logger$(OBJ_SUFF) \
	         $(OBJ)/Assembler$(OBJ_SUFF)

all: $(OUT) $(BIN)/main

OBJ_DEPS = $(MATHVM_OBJ) $(ASMJIT_OBJ) $(OBJ)/main$(OBJ_SUFF) \
	  $(OBJ)/JITCompiler$(OBJ_SUFF)  $(OBJ)/Interpreter$(OBJ_SUFF) \
	  $(OBJ)/TranslationUtils$(OBJ_SUFF) $(OBJ)/SymbolVisitor$(OBJ_SUFF) \
	  $(OBJ)/TypeVisitor$(OBJ_SUFF) $(OBJ)/CodeVisitor$(OBJ_SUFF) \
	  $(OBJ)/Executable$(OBJ_SUFF)

INCLUDE_DEPS = ExecutionEnv.h JITCompiler.h Interpreter.h \
	  TranslationUtils.h SymbolVisitor.h TypeVisitor.h \
	  CodeVisitor.h Executable.h Translator.h \
	  SymbolStack.h TranslationException.h parse.h \
	  ByteCodeAnnotations.h

$(BIN)/main: dirs $(INCLUDE_DEPS) $(OBJ_DEPS)
	$(CXX) $(USER_FLAGS) -o $@ $(THREAD_LIB) $(OBJ_DEPS)
	
dirs:
	mkdir -p ./build/debug
	mkdir -p  ./build/opt
