#include "CppCode.h"
#include <stdlib.h>
#include <iostream>

namespace mathvm {

Status* CppCode::execute(vector<Var*>&) {
    string result = "/tmp/mathvm_tmp_result"; // tmpnam?
    string callCmd =
        "g++ -std=c++11 -Ofast `sdl-config --cflags` -fPIC -D_POSIX_SOURCE -DMATHVM_WITH_SDL "
        + cppCodeFilename +
        " -o " + result +
        " -ldl -rdynamic `sdl-config --libs` build/opt/utils.o";
    system(("echo \"" + callCmd + "\"").c_str());
    system(callCmd.c_str());
    system(result.c_str());
    return Status::Ok();
}

}
