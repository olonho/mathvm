ROOT   = .
VM_ROOT = ../../../..
USER_DEFS = 
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/VisitorSourcePrinter$(OBJ_SUFF) $(OBJ)/Translator$(OBJ_SUFF) $(OBJ)/Code$(OBJ_SUFF) $(OBJ)/ExecStack$(OBJ_SUFF)  $(OBJ)/MVException$(OBJ_SUFF) $(OBJ)/PSVarTable$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB0 = $(BIN)/lab23

all: $(LAB0)

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/Translator$(OBJ_SUFF) $(OBJ)/VisitorSourcePrinter$(OBJ_SUFF) $(OBJ)/Code$(OBJ_SUFF) $(OBJ)/ExecStack$(OBJ_SUFF)  $(OBJ)/MVException$(OBJ_SUFF) $(OBJ)/PSVarTable$(OBJ_SUFF) : VisitorSourcePrinter.h Translator.h Code.h ExecStack.h  MVException.h PSVarTable.h

$(LAB0): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

test: $(LAB1)
	./test.sh
