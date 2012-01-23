ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = -g3
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/AstToAsm$(OBJ_SUFF) $(OBJ)/Exceptions$(OBJ_SUFF) $(OBJ)/VarsSearcherVisitor$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB4 = $(BIN)/lab3

all: $(LAB3)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/AstToAsm$(OBJ_SUFF) $(OBJ)/Exceptions$(OBJ_SUFF) (OBJ)/VarsSearcherVisitor$(OBJ_SUFF) : AstToAsm.h Exceptions.h VarsSearcherVisitor.h

$(LAB3): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB4)
	./test.sh
