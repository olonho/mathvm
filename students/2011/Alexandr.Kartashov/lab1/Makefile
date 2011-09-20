VM_ROOT = ../../../..

ROOT   = .
OUT    = $(ROOT)/build
OBJ    = $(OUT)/obj
BIN    = $(OUT)/bin
MATHVM = $(BIN)/lab1
MATHVMTGZ = ../MathVM.tgz

CC         = gcc
CXX        = g++
CFLAGS     = -ggdb3 -Wall -Werror -D_REENTRANT
INCLUDE    = -I$(VM_ROOT)/include
VM_INCLUDE = -I$(VM_ROOT)/vm
DEFS       = -DMATHVM_POSIX
THREAD_LIB = -lpthread
OBJ_SUFF   = o

ALL_CXXFLAGS = -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE)


MATHVM_INCLUDES = 			\
	$(VM_ROOT)/include/ast.h	\
	$(VM_ROOT)/include/mathvm.h	\
	$(VM_ROOT)/include/visitors.h 	\
        $(VM_ROOT)/vm/scanner.h 	\
	$(VM_ROOT)/vm/parser.h

MATHVM_OBJ = 				\
        $(OBJ)/ast.o 			\
        $(OBJ)/interpreter.o 		\
        $(OBJ)/mathvm.o 		\
        $(OBJ)/parser.o 		\
        $(OBJ)/scanner.o
#        $(OBJ)/translator.o

LAB1_OBJ = 				\
	$(OBJ)/main.o			\
	$(OBJ)/translator.o

ALL_OBJ = $(MATHVM_OBJ) $(LAB1_OBJ)


all: $(MATHVM)

$(OBJ)/%.o: $(VM_ROOT)/vm/%.cpp $(MATHVM_INCLUDES)
	$(CXX) $(ALL_CXXFLAGS) $< -o $@

$(OBJ)/main.o: main.cpp
	$(CXX) $(ALL_CXXFLAGS) $< -o $@

$(OBJ)/translator.o: translator.cpp
	$(CXX) $(ALL_CXXFLAGS) $< -o $@

$(MATHVM): $(OUT) $(ALL_OBJ)
	$(CXX) -o $@ $(ALL_OBJ)

tar:
	rm -f $(MATHVMTGZ)
	tar czf $(MATHVMTGZ) ../MathVM


$(ROOT)/$(OUT):
	mkdir -p $(OUT)
	mkdir -p $(OBJ)
	mkdir -p $(BIN)

clean:
	rm -rf $(OUT)

test: $(MATHVM)
	./test.sh