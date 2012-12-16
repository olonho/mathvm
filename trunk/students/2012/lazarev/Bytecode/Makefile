ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/BytecodeGenerationVisitor$(OBJ_SUFF) \
   $(OBJ)/InterpreterCodeImpl$(OBJ_SUFF) \
   $(OBJ)/BytecodeTranslatorImpl$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk
CFLAGS += -march=native

INTERPRETER = $(BIN)/interpreter

all: $(INTERPRETER)

$(INTERPRETER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(OBJ)/%$(OBJ_SUFF): \
	BytecodeGenerationVisitor.h \
	BytecodeGenerationVisitor.cpp \
	InterperterCodeImpl.h \
	InterperterCodeImpl.cpp \
	BytecodeTranslatorImpl.cpp \
	main.cpp
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

