ROOT   = .
VM_ROOT = ./../../../..

OPT=1

USER_OBJ = \
   $(OBJ)/AstToMCConverter$(OBJ_SUFF)\
   $(OBJ)/main$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
