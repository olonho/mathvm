#ifndef BYTECODE_VISITOR_H
#define BYTECODE_VISITOR_H
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "context.h"
#include <iostream>
#include <map>
#include "interpretator.h"
#include "exceptions.h"

using namespace mathvm;
using std::ostream;
using std::cout;
using std::map;
using std::make_pair;


//TODO
//destructor remove all context
class ByteCodeVisitor: public AstVisitor
{
    typedef pair<int16_t, Var> Variable;
    typedef map <string, Variable> VariableMap;


    //with unique id to all program
    typedef bool native;
    typedef map <string, int16_t> FunctionMap;
    typedef map <int16_t, native> Function;
    void visitTop();
#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    public:
        ByteCodeVisitor(AstFunction * top_m, InterpretCode * kCode): top(top_m), code(kCode)
    {
        initMaps();
        visitTop();
    }

    Code * getCode()
    {
        return code;
    }

private:
    BytecodeFunction * currenFunction;
    vector <VariableMap> allVariables;
    vector <FunctionMap> allFunctions;
    static int16_t funcIdx;
    Function nativeFunctions;


    VarType resultType;

    pair<int16_t, int16_t> getVarIdx(Context * context, string varName) const;
    int getFuncIdx(Context * context, string varName) const;

    VarType getTypeToBinOperation(VarType left, VarType right);

    AstFunction * top;
    Code * code;
    Context * current;
    static int16_t currentContext;

private:
    void typeConverter(VarType typeOut, VarType curType);
    void pushDoubleOnStack(double value);
    void pushIntOnStack(int64_t value);
    void pushStringOnStack(string value);
    void storeValueFromStack(TokenKind kind, VarType typeOut, VarType curType, pair<int16_t, int16_t> var);
    void loadValueToStack(VarType typeOut, pair<int16_t, int16_t> var);
    void printValueFromStack(VarType typeOut);
    void arithOperation(TokenKind kind, VarType resultType);
    void unaryOperations(VarType resultType, TokenKind kind);
    void logicalOperations(TokenKind kind, VarType resultType);
    void comparateOperation(TokenKind kind, VarType resultType);
    void call(int16_t idx, bool native);
    Bytecode * byteCode()
    {
        return currenFunction->bytecode();
    }

    // TODO
    // move to utills
    map <VarType, Instruction> loadMap;
    map <VarType, Instruction> storeMap;
    map <VarType, Instruction> pushMap;
    map <VarType, Instruction> printMap;
    map <VarType, Instruction> sumMap;
    map <VarType, Instruction> subMap;
    map <VarType, Instruction> multMap;
    map <VarType, Instruction> divMap;
    map <VarType, Instruction> zeroMap;
    map <VarType, Instruction> negateMap;
    map <VarType, Instruction> unitMap;
    map <VarType, Instruction> uunitMap;
    map <VarType, Instruction> compareMap;

    void initMaps();

private:
    void initContext(BlockNode * node);
};

Status * BytecodeTranslatorImpl::translate(const string & program,
                                           Code ** code)
{
    Parser parser;
    Status * status = parser.parseProgram(program);

    if (status && status->isError()) return status;

    InterpretCode kc;

    try
    {
        ByteCodeVisitor result(parser.top(), &kc);
        *code = result.getCode();
        Code::FunctionIterator it(*code);
        while(it.hasNext()){
            BytecodeFunction *bcF = (BytecodeFunction*)it.next();
            cout << endl<<"nameF = " <<bcF->name() << " idx = " << bcF->id()<<endl;
            bcF->bytecode()->dump(cout);
            cout << endl;
        }
    }
    catch (Exception ex)
    {
        return Status::Error(ex.what());
    }

    //vector<Var *> vars;
    return Status::Ok();
}

//Translator * Translator::create(const string & impl)
//{
//    if (impl == "intepreter")
//    {
//        return new BytecodeTranslatorImpl();
//    }
//    else
//    {
//        return NULL;
//    }
//}


#endif // BYTECODE_VISITOR_H
