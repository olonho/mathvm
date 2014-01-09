//
// Created by Vadim Lomshakov on 09/01/14.
// Copyright (c) 2014 spbau. All rights reserved.
//

#ifndef __MemoryManager_H_
#define __MemoryManager_H_

#include <map>
#include <set>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include "mathvm.h"
#include "InterpreterImpl.h"

#define ssysint_t int64_t
#define MAX_HEAP_SZ (1024 * 2)

namespace mathvm {

struct RefMetaData {
  TypeTag of;
  uint32_t countElement;
  bool reached;

  RefMetaData():
    of(VT_INVALID),
    countElement(0),
    reached(false) {
  }

  RefMetaData(int64_t count, TypeTag ofT):
    of(ofT),
    countElement(count),
    reached(false) {
    assert(of == VT_DOUBLE || of == VT_INT || of == VT_REF);
  }

  size_t size() const {
    return elementSize() * countElement;
  }

  size_t elementSize() const {
    size_t elemSz = 0;
    switch (of) {
      case VT_DOUBLE:
        elemSz = sizeof(int64_t);
        break;
      case VT_INT:
        elemSz = sizeof(double);
        break;
      case VT_REF:
        elemSz = sizeof(ssysint_t);
        break;
      default:
        assert(false);
    }
    return elemSz;
  }

  bool elementPrimitiveType() const {
    return of == VT_INT || of == VT_DOUBLE;
  }
};


class MSMemoryManager {
  map<ssysint_t, RefMetaData> _allocatedAddresses;
  uint64_t _allUsedMem;

  InterpreterImpl* _interpreter;
public:

  inline
  void* alloc(RefMetaData info) {
    _allUsedMem += info.size();
    if (_allUsedMem > MAX_HEAP_SZ)
      collect();

    assert(_allUsedMem <= MAX_HEAP_SZ);

    void * mem = malloc((size_t) info.size());
    memset(mem, 0, info.size());
    assert(mem != 0);

    _allocatedAddresses[(ssysint_t)mem] = info;
    return mem;
  }

  static MSMemoryManager & mm();

  void setClient(InterpreterImpl *interpreter) {
    _interpreter = interpreter;
  }

private:
  inline
  void collect() {
    assert(_interpreter != 0);
    set<ssysint_t> rs = _interpreter->buildRootSet();
    for (set<ssysint_t>::iterator i = rs.begin(); i != rs.end(); ++i)
      mark(*i);

    // marks
    while (!rs.empty()) {
      ssysint_t o = *rs.begin();
      rs.erase(rs.begin());

      RefMetaData& metaData = _allocatedAddresses[o];
      if (!metaData.elementPrimitiveType()) {

        for (size_t i = 0; i != metaData.countElement; ++i) {
          ssysint_t oo = o + i * metaData.elementSize();
          RefMetaData& newRoot = _allocatedAddresses[oo];
          if (!newRoot.reached) {
            newRoot.reached = true;
            rs.insert(oo);
          }
        }
      }
    }

    // sweeps
    for (map<ssysint_t, RefMetaData>::iterator it = _allocatedAddresses.begin();
         it != _allocatedAddresses.end(); ) {
      if (!it->second.reached) {
        _free(*it);
        _allocatedAddresses.erase(it++);
      } else {
        it->second.reached = false;
        ++it;
      }
    }
  }

  inline
  void mark(ssysint_t address) {
    _allocatedAddresses[address].reached = true;
  }

  inline
  void _free(pair<ssysint_t, RefMetaData> const& inf) {
    size_t sz = inf.second.size();
    assert(_allUsedMem >= sz);
    _allUsedMem -= sz;

    free((void*)inf.first);
  }

  MSMemoryManager():
    _allUsedMem(0),
    _interpreter(0) {
  }

  MSMemoryManager(MSMemoryManager&);
  MSMemoryManager& operator=(MSMemoryManager& );
};

}


#endif //__MemoryManager_H_
