ROOT = .
VM_ROOT = ../../../..

OPT=1
WITH_SDL=1

USER_OBJ = $(OBJ)/main$(OBJ_SUFF) \
		   $(OBJ)/machcode$(OBJ_SUFF) \
		   $(OBJ)/jitcompiler$(OBJ_SUFF) \
		   $(OBJ)/codeanalyzer$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

MVM = $(BIN)/mvm

all: $(MVM)

$(MVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
