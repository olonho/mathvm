ROOT   = .
VM_ROOT = ../../..
#USER_DEFS = OPT=opt
USER_OBJ = $(OBJ)/main$(OBJ_SUFF) $(OBJ)/ByteCodeGenerator$(OBJ_SUFF) $(OBJ)/MyInterpreter$(OBJ_SUFF) $(OBJ)/FirstPassVisitor$(OBJ_SUFF) $(OBJ)/GeneratorCommon$(OBJ_SUFF)

include $(VM_ROOT)/common.mk

LAB1 = $(BIN)/lab1

$(OBJ)/main$(OBJ_SUFF) $(OBJ)/ByteCodeGenerator$(OBJ_SUFF) : ByteCodeGenerator.h

all: $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) -o $@ $(MATHVM_OBJ) $(USER_OBJ) $(THREAD_LIB) 

