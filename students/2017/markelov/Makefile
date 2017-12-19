ROOT   = .
VM_ROOT = ../../..

OPT = 0
NO_JIT = 1
WITH_SDL = 1

USER_CFLAGS = -std=c++11

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

#USER_OBJ_SRC=main.cpp interpret.cpp mytranslator.cpp

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/interpret$(OBJ_SUFF) \
   $(OBJ)/privatetranslator$(OBJ_SUFF)
   
USER_OBJ_DEPS=$(USER_OBJ:.o=.d)

USER_DEPS=interpret.h translator.h code.h printer.h

include $(VM_ROOT)/common.mk

INCLUDE += -I$(ROOT)/include

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(USER_OBJ) $(MATHVM_OBJ)
	$(CXX) -o $@ $(USER_OBJ) $(MATHVM_OBJ) $(LIBS)




	
