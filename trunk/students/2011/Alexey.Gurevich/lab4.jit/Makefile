VM_ROOT = ../../../..
ROOT = .
USER_CFLAGS = -ggdb3 
WITH_SDL = 1
include $(VM_ROOT)/common.mk

all: $(OUT) $(BIN)/lab4

OBJ_DEPS = $(MATHVM_OBJ) $(OBJ)/main$(OBJ_SUFF) \
	  $(OBJ)/bytecoder$(OBJ_SUFF)  $(OBJ)/my_code$(OBJ_SUFF) \
	  $(OBJ)/my_translator$(OBJ_SUFF)

INCLUDE_DEPS = config.h bytecoder.h my_code.h \
	  my_translator.h

$(BIN)/lab4: dirs $(INCLUDE_DEPS) $(OBJ_DEPS)
	$(CXX) $(USER_FLAGS) -o $@ $(LIBS) $(OBJ_DEPS)