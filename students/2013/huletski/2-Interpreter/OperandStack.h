//
//  OperandStack.h
//  VM_2
//
//  Created by Hatless Fox on 11/10/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_2_OperandStack_h
#define VM_2_OperandStack_h

#include <vector>
#include <stack>

class OperandStack {
  
public:
  
  OperandStack():_data(), _index(-1) {
    _data.resize(2048);
  }
  
  inline void push(int64_t elem) {
    if (_data.size() <= ++_index) {
      _data.resize(_data.size() * 2);
    }
    _data[_index] = elem;
  }
  
  inline int64_t pop() { return _data[_index--]; }
  
  
private:
  std::vector<int64_t> _data;;
  int64_t _index;
};


#endif
