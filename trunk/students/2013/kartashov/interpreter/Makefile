ROOT = .
VM_ROOT = ../../../..

WITH_SDL=1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
		   $(OBJ)/bctranslator$(OBJ_SUFF) \
		   $(OBJ)/bcinterpreter$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MVM = $(BIN)/mvm

all: $(MVM) printer

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
	
printer: bcprinter.cpp $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $(OBJ)/bcprinter $(INCLUDE) bcprinter.cpp $(MATHVM_OBJ) $(OBJ)/bctranslator$(OBJ_SUFF) $(OBJ)/bcinterpreter$(OBJ_SUFF) $(LIBS)
