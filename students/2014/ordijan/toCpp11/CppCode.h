#ifndef _CPP_CODE_H
#define _CPP_CODE_H

#include "mathvm.h"

namespace mathvm {

struct CppCode: public Code {
  string cppCodeFilename;
  virtual Status* execute(vector<Var*>& vars);
};

}

#endif // _CPP_CODE_H
