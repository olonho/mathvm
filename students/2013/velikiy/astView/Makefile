ROOT   = .
VM_ROOT = ../../../..
USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator$(OBJ_SUFF) \
   $(OBJ)/printastvisitor$(OBJ_SUFF)


include $(VM_ROOT)/common.mk

ASTVIEW = $(BIN)/astview

all: $(ASTVIEW)

$(ASTVIEW): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(LIBS)

$(OBJ)/main$(OBJ_SUFF): main.cpp printastvisitor.cpp
