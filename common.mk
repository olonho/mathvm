OUT    = $(ROOT)/build
ifeq ($(OPT),)
 CFLAGS = -g
 OBJ    = $(OUT)/debug
 BIN    = $(OUT)/debug
else
 OBJ    = $(OUT)/opt
 BIN    = $(OUT)/opt
 CFLAGS = -O2
endif

MATHVM = $(BIN)/mvm
MATHVMTGZ = ../MathVM.tgz

CC         = gcc
CXX        = g++
CFLAGS     += -Wall -Werror -D_REENTRANT $(USER_CFLAGS)
INCLUDE    = -I$(VM_ROOT)/include
VM_INCLUDE = -I$(VM_ROOT)/vm
USER_INCLUDE = -I$(ROOT)
DEFS       = -DMATHVM_POSIX $(USER_DEFS)
THREAD_LIB = -lpthread $(USER_LIBS)
OBJ_SUFF   = .o

MATHVM_OBJ = \
        $(OBJ)/ast$(OBJ_SUFF) \
        $(OBJ)/interpreter$(OBJ_SUFF) \
        $(OBJ)/mathvm$(OBJ_SUFF) \
        $(OBJ)/parser$(OBJ_SUFF) \
        $(OBJ)/scanner$(OBJ_SUFF) \
        $(OBJ)/utils$(OBJ_SUFF)

default: $(OUT) all

tar:
	rm -f $(MATHVMTGZ)
	tar czf $(MATHVMTGZ) ../MathVM

$(OBJ)/%$(OBJ_SUFF): $(VM_ROOT)/vm/%.cpp \
	$(OUT) \
	$(VM_ROOT)/include/ast.h $(VM_ROOT)/include/mathvm.h \
        $(VM_ROOT)/include/visitors.h \
        $(VM_ROOT)/vm/scanner.h $(VM_ROOT)/vm/parser.h \
	$(VM_ROOT)/common.mk
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

$(OBJ)/%$(OBJ_SUFF): $(ROOT)/%.cpp \
	$(OUT) \
	$(VM_ROOT)/include/ast.h $(VM_ROOT)/include/mathvm.h \
	$(VM_ROOT)/include/visitors.h \
	$(VM_ROOT)/vm/scanner.h $(VM_ROOT)/vm/parser.h \
	$(VM_ROOT)/common.mk $(USER_DEPS)
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

$(OUT):
	mkdir -p $(OUT)
	mkdir -p $(OBJ)
	mkdir -p $(BIN)

clean:
	rm -rf $(OUT)
