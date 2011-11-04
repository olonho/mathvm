#include <map>

#include "Runtime.h"

// ================================================================================

typedef void (*TopFunc)(size_t*);

namespace mathvm {
  NativeFunction* Runtime::createFunction(AstFunction* af) {
    NativeFunction* nf = new NativeFunction(af);
    _functions.push_back(nf);

    if (af->name() == AstFunction::top_name) {
      _topf = nf;
      
      Scope::VarIterator vi(af->node()->body()->scope());
      size_t p = 0;
      while (vi.hasNext()) {
        vi.next();
        p++;
      }      


      vi = Scope::VarIterator(af->node()->body()->scope());
      while (vi.hasNext()) {
        AstVar* arg = vi.next();
        _topArgMap[arg->name()] = p;
        p--;
      }      
    }

    return _functions.back();
  }

  static size_t squash(const Var* v) {
    double d;
    int64_t i;

    switch (v->type()) {
    case VT_INT:
      i = getIntValue();
      return i;

    case VT_DOUBLE:
      d = getDoubleValue();
      return *(size_t*)d;
      
    default:
      ABORT("Not supported");
    }

    return 0;
  }

  static void unsquash(size_t val, Var* var) {
    double d;
    int64_t i;

    switch (v->type()) {
    case VT_INT:
      var->setIntValue((int64_t)val);
      break;

    case VT_DOUBLE:
      var->setDoubleValue(*(double*)val);
      break;
      
    default:
      ABORT("Not supported");
    }
  }

  Status* Runtime::execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args) { 
    size_t* argFrame = new size_t[_topArgMap.size()];

    for (int i = 0; i < args.size(); ++i) {
      if (_topArgMap.find(args[i]->name()) == _topArgMap.end()) {
        ABORT("Invalid argument name");
      }

      argFrame[_topArgMap[args[i]->name()]] = squash(args[i]);
    }

    NativeCode* c = _topf->code();
    TopFunc f = (TopFunc)c->x86code();
    f(argFrame);

    for (int i = 0; i < args.size(); ++i) {
      unsquash(argFrame[_topArgMap[args[i]->name()]], args[i]);
    }

    delete [] argFrame;

    return NULL;
  }
}
