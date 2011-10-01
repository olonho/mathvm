VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
#-DNDEBUG

include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/gen_code

DEPS = $(MATHVM_OBJ) parse.h CodeVisitor.h $(OBJ)/CodeVisitor.o MyCode.h $(OBJ)/MyCode.o

$(BIN)/gen_code: $(DEPS) $(OBJ)/gen_code.o
	$(CXX) $(USER_FLAGS) -o $@ $(MATHVM_OBJ) $(THREAD_LIB) $(OBJ)/gen_code.o $(OBJ)/CodeVisitor.o $(OBJ)/MyCode.o

