ROOT   = .
VM_ROOT = ./../../../..

WITH_SDL=true
OPT=true

USER_OBJ = \
   $(OBJ)/AstToMCConverter$(OBJ_SUFF)\
   $(OBJ)/main$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

USER_CFLAGS = -DFAST_DIV
MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
