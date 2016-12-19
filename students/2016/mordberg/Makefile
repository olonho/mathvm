ROOT   = .
VM_ROOT = ../../..
TEST_ROOT = $(VM_ROOT)/tests

USER_CFLAGS = -std=c++11 -Ofast

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator_impl$(OBJ_SUFF) \
   $(OBJ)/print_translator$(OBJ_SUFF) \
   $(OBJ)/interpreter_code_impl$(OBJ_SUFF) \
   $(OBJ)/bytecode_interpreter_context$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator_impl$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator_context$(OBJ_SUFF) \
   $(OBJ)/ast_node_type_resolver$(OBJ_SUFF)


include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)

test: $(MATHVM)
	$(TEST_ROOT)/run.py -t $(TEST_ROOT) -e $(MATHVM)
	$(TEST_ROOT)/run.py -t $(TEST_ROOT)/additional/ -e $(MATHVM)
	$(TEST_ROOT)/run.py -t $(TEST_ROOT)/optional/ -e $(MATHVM)
