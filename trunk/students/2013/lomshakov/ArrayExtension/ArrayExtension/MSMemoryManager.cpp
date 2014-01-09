//
// Created by Vadim Lomshakov on 09/01/14.
// Copyright (c) 2014 spbau. All rights reserved.
//
#include "MSMemoryManager.h"

namespace mathvm {

  MSMemoryManager &MSMemoryManager::mm() {
    static MSMemoryManager memoryManager;
    return memoryManager;
  }
}
