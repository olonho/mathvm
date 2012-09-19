OUT     = $(ROOT)/build
CFLAGS  = -g -Wall -Werror -D_REENTRANT $(USER_CFLAGS)
OBJ     = $(OUT)/debug
BIN     = $(OUT)/debug

CXX          = g++
INCLUDE      = -I$(VM_ROOT)/include -I$(VM_ROOT)/vm
USER_INCLUDE = -I$(ROOT)
DEFS         = -D_POSIX_SOURCE -DMATHVM_POSIX
OBJ_SUFF     = .o

MATHVM_OBJ = \
        $(OBJ)/ast$(OBJ_SUFF) \
        $(OBJ)/mathvm$(OBJ_SUFF) \
        $(OBJ)/parser$(OBJ_SUFF) \
        $(OBJ)/scanner$(OBJ_SUFF) \
        $(OBJ)/utils$(OBJ_SUFF) \

default: $(OBJ)/.dir all

$(OBJ)/%$(OBJ_SUFF): $(VM_ROOT)/vm/%.cpp \
	$(OBJ)/.dir \
	$(VM_ROOT)/include/ast.h \
	$(VM_ROOT)/include/mathvm.h \
        $(VM_ROOT)/include/visitors.h \
        $(VM_ROOT)/vm/scanner.h $(VM_ROOT)/vm/parser.h \
        $(ROOT)/common.mk $(USER_DEPS)
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(USER_INCLUDE) $< -o $@

$(OBJ)/%$(OBJ_SUFF): $(ROOT)/%.cpp \
	$(OBJ)/.dir \
	$(VM_ROOT)/include/ast.h $(VM_ROOT)/include/mathvm.h \
	$(VM_ROOT)/include/visitors.h \
	$(VM_ROOT)/vm/scanner.h $(VM_ROOT)/vm/parser.h \
	$(ROOT)/common.mk $(USER_DEPS)
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(USER_INCLUDE) $< -o $@

$(OBJ)/.dir:
	mkdir -p $(OUT)
	mkdir -p $(OBJ)
	mkdir -p $(BIN)
	touch $@

clean:
	rm -rf $(OUT)

.PHONY : clean dirs all default
