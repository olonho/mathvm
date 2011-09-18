ROOT   = .
OUT    = $(ROOT)/build
OBJ    = $(OUT)/obj
BIN    = $(OUT)/bin
MATHVM = $(BIN)/mvm
MATHVMTGZ = ../MathVM.tgz

CC         = gcc
CXX        = g++
CFLAGS     = -g -Wall -Werror -D_REENTRANT
INCLUDE    = -I$(ROOT)/include
VM_INCLUDE = -I$(ROOT)/vm
DEFS       = -DMATHVM_POSIX
THREAD_LIB = -lpthread
OBJ_SUFF   = o

MATHVM_OBJ = \
        $(OBJ)/ast.o \
        $(OBJ)/interpreter.o \
        $(OBJ)/mathvm.o \
        $(OBJ)/parser.o \
        $(OBJ)/scanner.o \
        $(OBJ)/translator.o

all: $(MATHVM)

tar:
	rm -f $(MATHVMTGZ)
	tar czf $(MATHVMTGZ) ../MathVM

$(OBJ)/%.o: $(ROOT)/vm/%.cpp \
	$(ROOT)/include/ast.h $(ROOT)/include/mathvm.h $(ROOT)/include/visitors.h \
        $(ROOT)/vm/scanner.h $(ROOT)/vm/parser.h
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(OBJ)/main.o
	$(CXX) -o $@ $(MATHVM_OBJ) $(OBJ)/main.o $(THREAD_LIB)

$(ROOT)/$(OUT):
	mkdir -p $(OUT)
	mkdir -p $(OBJ)
	mkdir -p $(BIN)

clean:
	rm -rf $(OUT)
