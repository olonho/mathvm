#ifndef ERROR_REPORTER_HPP
#define ERROR_REPORTER_HPP 

#include "mathvm.h"

#include <cassert>
#include <cstdlib>

namespace mathvm {

class ErrorReporter {
  Status* status_;
public:
  ErrorReporter(): status_(Status::Ok()) {}
  
  ~ErrorReporter() {
    if (status_ != NULL) {
      delete status_;
    }
  }

  Status* release() {
    assert(status_ != NULL);
    Status* status = status_;
    status_ = NULL;
    return status;
  }

  bool isOk() {
    return status_->isOk();
  }

  bool isError() {
    return status_->isError();
  }

  void error(const char* msg) {
    setStatus(Status::Error(msg));
  }

  void error(const char* msg, AstNode* node) {
    setStatus(Status::Error(msg, node->position()));
  }
private:
  void setStatus(Status* status) {
    delete status_;
    status_ = status;
  } 
};

}

#endif