OUT    = $(ROOT)/build
ifeq ($(OPT),)
 CFLAGS = -g3
 OBJ    = $(OUT)/debug
 BIN    = $(OUT)/debug
else
 OBJ    = $(OUT)/opt
 BIN    = $(OUT)/opt
 CFLAGS = -O2
endif
OS     := $(shell uname -s)

MATHVM = $(BIN)/mvm
MATHVMTGZ = ../MathVM.tgz

CXX        ?= g++
CFLAGS     += -Wall -Werror -D_REENTRANT -fPIC $(USER_CFLAGS)
LIBS_ROOT = $(VM_ROOT)/libs
ASMJIT_CFLAGS = -Wno-error
INCLUDE    = -I$(VM_ROOT)/include -I$(LIBS_ROOT)
VM_INCLUDE = -I$(VM_ROOT)/vm
ASMJIT_INCLUDE = -I$(LIBS_ROOT)/AsmJit
USER_INCLUDE = -I$(ROOT)
DEFS       = $(USER_DEFS) -D_POSIX_SOURCE
THREAD_LIB = -lpthread 
OBJ_SUFF   = .o
LIBS = $(USER_LIBS) $(THREAD_LIB)


ifneq (,$(findstring MINGW,$(OS)))
 DEFS += -DMATHVM_WIN
else
 DEFS += -DMATHVM_POSIX
 LIBS += -ldl
endif

ifneq (,$(findstring Darwin,$(OS)))
 ifneq ($(WITH_SDL),)
  DEFS += -I/opt/local/include
  LIBS += -L/opt/local/lib -framework cocoa -lSDLmain
 endif
 DEFS += -D_DARWIN_C_SOURCE
endif

ifneq (,$(findstring Linux,$(OS)))
 LIBS += -rdynamic
endif

ifneq ($(WITH_SDL),)
  DEFS += -DMATHVM_WITH_SDL
  LIBS += $(shell sdl-config --libs)
  CFLAGS += $(shell sdl-config --cflags)
endif

ifneq ($(NO_JIT),1)
ASMJIT_OBJ = \
        $(OBJ)/AssemblerX86X64$(OBJ_SUFF) \
        $(OBJ)/CodeGenerator$(OBJ_SUFF) \
        $(OBJ)/Compiler$(OBJ_SUFF) \
        $(OBJ)/CompilerX86X64$(OBJ_SUFF) \
        $(OBJ)/CpuInfo$(OBJ_SUFF) \
        $(OBJ)/Defs$(OBJ_SUFF) \
        $(OBJ)/DefsX86X64$(OBJ_SUFF) \
        $(OBJ)/Logger$(OBJ_SUFF) \
        $(OBJ)/MemoryManager$(OBJ_SUFF) \
        $(OBJ)/OperandX86X64$(OBJ_SUFF) \
        $(OBJ)/Platform$(OBJ_SUFF) \
        $(OBJ)/Util$(OBJ_SUFF)
else
ASMJIT_OBJ = 
endif

MATHVM_OBJ = \
        $(ASMJIT_OBJ) \
        $(OBJ)/ast$(OBJ_SUFF) \
        $(OBJ)/interpreter$(OBJ_SUFF) \
        $(OBJ)/mathvm$(OBJ_SUFF) \
        $(OBJ)/parser$(OBJ_SUFF) \
        $(OBJ)/scanner$(OBJ_SUFF) \
        $(OBJ)/utils$(OBJ_SUFF) \

default: $(OBJ)/.dir all

tar:
	rm -f $(MATHVMTGZ)
	tar czf $(MATHVMTGZ) ../MathVM


$(OBJ)/%$(OBJ_SUFF): $(VM_ROOT)/vm/%.cpp \
	$(OBJ)/.dir \
	$(VM_ROOT)/include/ast.h $(VM_ROOT)/include/mathvm.h \
        $(VM_ROOT)/include/visitors.h \
        $(VM_ROOT)/vm/scanner.h $(VM_ROOT)/vm/parser.h \
	$(VM_ROOT)/common.mk
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

$(OBJ)/%$(OBJ_SUFF): $(ROOT)/%.cpp \
	$(OBJ)/.dir \
	$(VM_ROOT)/include/ast.h $(VM_ROOT)/include/mathvm.h \
	$(VM_ROOT)/include/visitors.h \
	$(VM_ROOT)/vm/scanner.h $(VM_ROOT)/vm/parser.h \
	$(VM_ROOT)/common.mk $(USER_DEPS)
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

$(OBJ)/%$(OBJ_SUFF): $(LIBS_ROOT)/AsmJit/%.cpp $(OBJ)/.dir
	$(CXX) -c $(DEFS) $(CFLAGS) $(ASMJIT_CFLAGS) $(INCLUDE) $(ASMJIT_INCLUDE) $< -o $@

$(OBJ)/.dir:
	mkdir -p $(OUT)
	mkdir -p $(OBJ)
	mkdir -p $(BIN)
	touch $@

clean:
	rm -rf $(OUT)

.PHONY : clean dirs all tar default

