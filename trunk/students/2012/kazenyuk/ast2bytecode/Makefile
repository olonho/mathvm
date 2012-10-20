ROOT   = .
VM_ROOT = ../../../..
# USER_OBJ = \
#    $(OBJ)/jit$(OBJ_SUFF) \
#    $(OBJ)/main$(OBJ_SUFF) \
#    $(OBJ)/translator$(OBJ_SUFF)
USER_OBJ = $(OBJ)/main$(OBJ_SUFF)


include $(VM_ROOT)/common.mk

AST2BYTECODE = $(BIN)/ast2bytecode

all: $(AST2BYTECODE)

$(AST2BYTECODE): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

#$(OBJ)/main$(OBJ_SUFF): converter.hpp utils.hpp
