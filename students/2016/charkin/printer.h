#pragma once

#include "visitors.h"

namespace mathvm {
    class Printer : public AstBaseVisitor {
    public:
        Printer(std::ostream &out_stream)
            :_out_stream(out_stream)
            , _depth(-1)
        {}
#define VISITOR_FUNCTION(type, name) \
  virtual void visit##type(type* node);

  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    private:
        std::ostream &_out_stream;
        int _depth;
        std::string indent();
        std::string escape(char c);
    };
}

