ROOT   = .
VM_ROOT = ./../../..

ifneq ($(NO_JIT), 1)
JIT_OBJ = $(OBJ)/jit$(OBJ_SUFF)
else
JIT_OBJ =
endif

USER_OBJ = \
   $(JIT_OBJ) \
   $(OBJ)/main$(OBJ_SUFF) \
   $(OBJ)/translator$(OBJ_SUFF) \
   $(OBJ)/bcg$(OBJ_SUFF) \
   $(OBJ)/bci$(OBJ_SUFF) 
#   $(OBJ)/bda$(OBJ_SUFF)

OPT = yes
CXX = g++
USER_CFLAGS = -std=c++11 -fno-inline-functions

NOP=@touch -c

include $(VM_ROOT)/common.mk


INCLUDE += -I$(ROOT)/include -I/usr/lib/c++/v1


$(OBJ)/%$(OBJ_SUFF): 	$(ROOT)/%.cpp		\
			$(OBJ)/.dir	        \
	$(CXX) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(VM_INCLUDE) $< -o $@

$(ROOT)/bcg.cpp:        $(ROOT)/include/bcg.h   \
			$(ROOT)/include/be.h
		$(NOP) $@

$(ROOT)/bci.cpp:        $(ROOT)/include/bci.h	\
			$(ROOT)/include/be.h
		$(NOP) $@

MATHVM = $(BIN)/mvm

all: $(MATHVM)

$(MATHVM): $(OUT) $(MATHVM_OBJ) $(USER_OBJ)
	$(CXX) $(CFLAGS) $(USER_OBJ) $(MATHVM_OBJ) $(LIBS) -o $@ 
