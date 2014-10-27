#include "jit.h"
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

#include <AsmJit/AsmJit.h>

using namespace AsmJit;
using namespace std;

namespace mathvm {

MachCodeImpl::MachCodeImpl() : _code(0) {
}

MachCodeImpl::~MachCodeImpl() {
  MemoryManager::getGlobal()->free(_code);
}

Status* MachCodeImpl::execute(vector<Var*>& vars) {
  int result = function_cast<int (*)()>(_code)();
  cout << "returned " << result << endl;
  return Status::Ok();
}

MachCodeFunction* MachCodeImpl::functionByName(const string& name) {
    return dynamic_cast<MachCodeFunction*>(Code::functionByName(name));
}

MachCodeFunction* MachCodeImpl::functionById(uint16_t id) {
    return dynamic_cast<MachCodeFunction*>(Code::functionById(id));
}

void MachCodeImpl::error(const char* format, ...) {
}

class MachCodeGenerator : public AstVisitor {
    AstFunction* _top;
    MachCodeImpl* _code;
    Assembler _;
public:
    MachCodeGenerator(AstFunction* top,
                      MachCodeImpl* code) :
    _top(top), _code(code) {
    }

    Status* generate();

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node) {}

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

Status* MachCodeGenerator::generate() {
  // Prologue.
  _.push(nbp);
  _.mov(nbp, nsp);

  // Return value.
  _.mov(nax, 239);

  // Epilogue.
  _.mov(nsp, nbp);
  _.pop(nbp);
  _.ret();

  _code->setCode(_.make());

  return Status::Ok();
}

Status* MachCodeTranslatorImpl::translateMachCode(const string& program,
                                                  MachCodeImpl* *result) {
  MachCodeImpl* code = 0;
  Parser parser;

  // Build an AST.
  Status* s = parser.parseProgram(program);
  if (s->isOk()) {
    delete s;
    code = new MachCodeImpl();
    MachCodeGenerator codegen(parser.top(), code);
    s = codegen.generate();
  }

  if (s->isError()) {
    *result = NULL;
    delete code;
  } else {
    *result = code;
  }
  return s;
}

MachCodeTranslatorImpl::MachCodeTranslatorImpl() {
}

MachCodeTranslatorImpl::~MachCodeTranslatorImpl() {
}

Status* MachCodeTranslatorImpl::translate(const string& program, Code* *result) {
    MachCodeImpl* code = 0;
    Status* status = translateMachCode(program, &code);

    if (status->isError()) {
        assert(code == 0);
        *result = 0;
        return status;
    }

    //code->disassemble();
    assert(code);
    *result = code;

    return status;
}

MachCodeFunction::MachCodeFunction(MachCodeImpl* owner, BytecodeFunction* bcfunc) :
  TranslatedFunction(bcfunc->name(), bcfunc->signature()) {
}

MachCodeFunction::~MachCodeFunction() {
}


Status* MachCodeFunction::execute(vector<Var*>* vars) {
  return Status::Ok();
}

void MachCodeFunction::disassemble(ostream& out) const {
}

}
