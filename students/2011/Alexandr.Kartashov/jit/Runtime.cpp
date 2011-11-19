#include <map>

#include "ast.h"
#include "Runtime.h"

// ================================================================================

typedef void (*TopFunc)(size_t*);

namespace mathvm {

  void Runtime::link() {
    size_t codeSize = 0;
    size_t base = 0;

    for (Functions::const_iterator fit = _functions.begin();
         fit != _functions.end();
         ++fit) {
      codeSize += (*fit)->code()->size();
    }

    _exec = (char*)mmap(NULL, codeSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    for (Functions::const_iterator fit = _functions.begin();
         fit != _functions.end();
         ++fit) {

      (*fit)->setStart(_exec + base);

      memcpy(_exec + base, (*fit)->code()->data(), (*fit)->code()->size());

      for (NativeFunction::Refs::const_iterator refIt = (*fit)->inRefs().begin();
           refIt != (*fit)->inRefs().end();
           ++refIt) {

        if (refIt->_fun->linked()) {
          // External offset
          int32_t* where = (int32_t*)(refIt->_fun->start() + refIt->_pos);

          //if ((char*)where < (*fit)->start() + (*fit)->code()->size()) {
            // The destination is known

            *where = (size_t)(*fit)->start() - ((size_t)where + 4) ;
            //}
        }
      }

      base += (*fit)->code()->size();

      for (NativeFunction::Refs::const_iterator refIt = (*fit)->outRefs().begin();
           refIt != (*fit)->outRefs().end();
           ++refIt) {

        // Internal offset

        if (refIt->_fun->linked() || refIt->_fun == *fit) {
          // We know where to jump
          int32_t* where = (int32_t*)((*fit)->start() + refIt->_pos);

          *where = (size_t)refIt->_fun->start() - ((size_t)where + 4);
        }
      }      
    }

    mprotect(_exec, codeSize, PROT_READ | PROT_EXEC);
  }

  // --------------------------------------------------------------------------------

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

  // --------------------------------------------------------------------------------

  static size_t squash(const Var* v) {
    double d;
    int64_t i;

    switch (v->type()) {
    case VT_INT:
      i = v->getIntValue();
      return i;

    case VT_DOUBLE:
      d = v->getDoubleValue();
      return *(size_t*)&d;
      
    default:
      ABORT("Not supported");
    }

    return 0;
  }

  static void unsquash(size_t val, Var* var) {
    switch (var->type()) {
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

    for (uint32_t i = 0; i < args.size(); ++i) {
      if (_topArgMap.find(args[i]->name()) == _topArgMap.end()) {
        ABORT("Invalid argument name");
      }

      argFrame[_topArgMap[args[i]->name()]] = squash(args[i]);
    }

    //NativeCode* c = _topf->code();
    TopFunc f = (TopFunc)_topf->start();
    f(argFrame);

    for (uint32_t i = 0; i < args.size(); ++i) {
      unsquash(argFrame[_topArgMap[args[i]->name()]], args[i]);
    }

    delete [] argFrame;

    return NULL;
  }

  // --------------------------------------------------------------------------------

  Runtime::~Runtime() {
    munmap(_exec, _codeSize);
  }
}
