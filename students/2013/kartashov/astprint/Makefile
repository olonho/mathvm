ROOT   = .
VM_ROOT = ../../../..

all: build

NO_JIT = 1
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) 
ASTPRINT = $(BIN)/astprint

include $(VM_ROOT)/common.mk

build: $(ASTPRINT)

$(ASTPRINT): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)
