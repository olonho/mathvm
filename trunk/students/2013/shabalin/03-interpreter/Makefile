ROOT   = .
VM_ROOT = ../../../../

USER_OBJ = \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/bytecode_translator$(OBJ_SUFF) \
   $(OBJ)/bytecode_interpreter$(OBJ_SUFF) \
   $(OBJ)/translator$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

#CXX=clang++ $(CXXFLAGS)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
