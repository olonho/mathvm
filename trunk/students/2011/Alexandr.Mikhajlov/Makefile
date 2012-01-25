VM_ROOT = ../../..
ROOT = .
USER_CFLAGS = -ggdb3 
WITH_SDL = 1
#USER_LIBS = -lSDL
#USER_DEFS = -DVERBOSE
#OPT = y
include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/main

OBJ_DEPS = $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) \
	  $(OBJ)/ByteCodeGenerator$(OBJ_SUFF)  $(OBJ)/FirstPassVisitor$(OBJ_SUFF) \
	  $(OBJ)/GeneratorCommon$(OBJ_SUFF) \
	  $(OBJ)/MyInterpreter$(OBJ_SUFF) $(OBJ)/NativeGenerator$(OBJ_SUFF)

INCLUDE_DEPS = ByteCodeGenerator.h FirstPassVisitor.h GeneratorCommon.h \
	  MyInterpreter.h NativeGenerator.h \

$(BIN)/main: dirs $(INCLUDE_DEPS) $(OBJ_DEPS)
	$(CXX) $(USER_FLAGS) -o $@ $(LIBS) $(OBJ_DEPS) -lSDL
	
dirs:
	mkdir -p ./build/debug
	mkdir -p  ./build/opt
