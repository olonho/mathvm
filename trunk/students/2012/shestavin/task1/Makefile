ROOT   = .
VM_ROOT = ../../../..

USER_OBJ = \
   $(OBJ)/main$(OBJ_SUFF) \

include $(VM_ROOT)/common.mk

ASTANALYZER = $(BIN)/astanalyzer

all: $(ASTANALYZER)

$(ASTANALYZER): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
