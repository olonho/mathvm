#include "CppCode.h"
#include <stdlib.h>

namespace mathvm {

Status* CppCode::execute(vector<Var*>&) {
    string result = "/tmp/mathvm_tmp_result"; // tmpnam?
    string callCmd = "g++ -std=c++11 -Ofast " + cppCodeFilename +
                     " -o " + result + " && " + result;
    system(callCmd.c_str());
    return Status::Ok();
}

}
