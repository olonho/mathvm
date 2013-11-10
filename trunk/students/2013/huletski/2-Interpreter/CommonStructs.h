//
//  CommonStructs.h
//  VM_2
//
//  Created by Hatless Fox on 10/26/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_2_CommonStructs_h
#define VM_2_CommonStructs_h

#include "mathvm.h"

using namespace mathvm;

struct VarInfo {
  uint16_t local_ind;
  uint16_t scope_id;
  VarType type;
  
  VarInfo(uint16_t l_i, uint16_t s_i, VarType t):
    local_ind(l_i), scope_id(s_i), type(t) {}
};

#endif
