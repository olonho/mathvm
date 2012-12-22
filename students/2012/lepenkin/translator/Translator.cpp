/* 
 * File:   TranslatorVisitor.cpp
 * Author: yarik
 * 
 * Created on October 2, 2012, 6:18 PM
 */

#include "Translator.h"
#include <parser.h>

TranslatorVisitor::TranslatorVisitor(Code** code)
     : _code(new SmartCode())
{
    *code = _code;
}

TranslatorVisitor::~TranslatorVisitor() {
}




void TranslatorVisitor::translate(AstFunction* top)
{
	WR_DEBUG("STARING TRANSLATING");

    _code->pushScope();

    top->node()->visit(this);


    ((BytecodeFunction*)_code->functionById(0))->bytecode()->addInsn(BC_STOP);
    //_code->currentBytecode()->addInsn(BC_STOP);

    WR_DEBUG("TRANSLATED: OK");
}

string kindRepr(TokenKind kind) 
{
    #define KIND_TO_STRING(type, str, p) if (kind == type) return str;
    FOR_TOKENS(KIND_TO_STRING)    
    #undef KIND_TO_STRING
    return "UNDEFINED TOKEN";
}


const string typeRepr(VarType type)
{
    switch(type) 
    {
        case VT_INVALID:
            return "INVALID";
        case VT_VOID:
            return "void";
        case VT_DOUBLE:
            return "double";
        case VT_INT:
            return "int";
        case VT_STRING:
            return "string";
        default:
            return "DE";
    }
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode* node)
{
    WR_DEBUG("visitBinaryOpNode: START");

	node->left()->visit(this);
    node->right()->visit(this);



    // i guess they pushed on stack

    VarType widest = varStack.top();



    switch (node->kind()) {
        case tSUB:
        	//TODO type check and conversion
        	_code->currentBytecode()->addInsn(BC_SWAP);
        	switch (widest)
        	{
        	    case VT_DOUBLE:
        	    	_code->currentBytecode()->addInsn(BC_DSUB);
        	    	break;
        	    case VT_INT:
        	    	_code->currentBytecode()->addInsn(BC_ISUB);
        	    	break;
       	        default:
        	        WR_ERROR("STRING BINARY OPS NOT IMPLEMENTED");
        	        break;
        	}
            break;
        case tADD:
        	switch (widest)
        	{
        	    case VT_DOUBLE:
        	    	_code->currentBytecode()->addInsn(BC_DADD);
        	     	break;
        	    case VT_INT:
        	        _code->currentBytecode()->addInsn(BC_IADD);
        	     	break;
        	    default:
        	         WR_ERROR("STRING BINARY OPS NOT IMPLEMENTED");
        	          break;
        	 }
        	break;
        case tMUL:
        	WR_ERROR("OOPS BINARY OPS IMPLEMENTED POORLY");

          	break;

        case tDIV:
        	WR_ERROR("OOPS BINARY OPS IMPLEMENTED POORLY");

         	break;

        default:
        	WR_ERROR("OOPS BINARY OPS IMPLEMENTED POORLY");
        	break;
    }

}


void TranslatorVisitor::visitBlockNodeBody(BlockNode* node)
{
	for (uint32_t i = 0; i < node->nodes(); ++i)
    {
        AstNode* curNode = node->nodeAt(i);
        curNode->visit(this);
    }
}



void TranslatorVisitor::visitBlockNode(BlockNode* node)
{
    WR_DEBUG("visitBlockNode: START");
    cout << "Scope: " << node->scope() << endl;
    visitScopeFuns(node->scope());
    visitScopeVars(node->scope());


	_code->pushScope();
    visitBlockNodeBody(node);
    _code->popScope();

    WR_DEBUG("visitBlockNode: END");
}

void TranslatorVisitor::visitCallNode(CallNode* node)
{

	FunInfo* info = _code->getFunIdByName(node->name());
    if (!info) {
    	cout << "Function " << node->name() << " ";
    	WR_ERROR("not found");
    }
	cout << "Calling fun ID: " << info->_id << endl;
	WR_DEBUG("visitCallNode NI");



/*
	cout << node->name() << "(";
    if (node->parametersNumber() > 0)
        node->parameterAt(0)->visit(this);
    for (uint32_t i = 1; i < node->parametersNumber(); ++i)
    {
        cout << ", ";
        node->parameterAt(i)->visit(this);
    }
    cout << ")";
    */
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
	_code->currentBytecode()->addInsn(BC_DLOAD);
    _code->currentBytecode()->addDouble(node->literal());
    varStack.push(VT_DOUBLE);

    //WR_DEBUG("visitDoubleLiteralNode: OK");
}

void TranslatorVisitor::visitForNode(ForNode* node)
{
	WR_ERROR("visitForNode NI");


	cout << endl << "for (";
    cout << node->var()->name() << " in ";    
    node->inExpr()->visit(this);
    cout << ")";
    node->body()->visit(this);
}

void TranslatorVisitor::visitFunctionNode(FunctionNode* node)
{
    WR_DEBUG("visitFunctionNode: START");

	BytecodeFunction* fun = _code->declareFunctionInCurrentScope(node);

    cout << "Added bytecode function. ID: " << fun->id() << " Name: " << fun->name() << endl;
    cout << "Searching fun by name: " << _code->getFunIdByName(fun->name())->_id << endl;

    _code->pushFunction(fun);
    _code->pushScope();


    uint32_t params = node->parametersNumber();
    for (uint32_t i = 0; i < params; ++i) {
    	string name = node->parameterName(i);
        VarType type = node->parameterType(i);
    	VarInfo* info = _code->declareVarInCurrentScope(new AstVar(name, type, 0));
        cout << "Declared function param: " << typeRepr(type) << " " << name << endl;
        cout << "               CTX: " << info->_context << " ID: " << info->_id << endl;
     }

     // was visitScopeVars and Funs

     node->body()->visit(this);

    _code->popFunction();
    _code->popScope();

    WR_DEBUG("visitFunctionNode: OK");
}


// int x; ?

// load on TOS ?
void TranslatorVisitor::visitLoadNode(LoadNode* node)
{
	WR_DEBUG("visitLoadNode: START");

	const AstVar* var = node->var();
	VarInfo* info = _code->getVarInfoByName(var->name());
    if (!info) {
        WR_ERROR("Var not found");
    }

	Instruction ins;
	switch (var->type()) {
	    case VT_INT:
		    ins = BC_LOADCTXIVAR;
		    break;
	    case VT_DOUBLE:
	    	ins = BC_LOADCTXDVAR;
	    	break;
	    case VT_STRING:
	    	ins = BC_LOADCTXSVAR;
	        break;
	    default:
	    	WR_ERROR("Invalid loading type");
	    	break;
	}

    _code->currentBytecode()->addInsn(ins);
    _code->currentBytecode()->addInt16(info->_context);
    _code->currentBytecode()->addInt16(info->_id);

    varStack.push(var->type());

    WR_DEBUG("visitLoadNode: OK");
}



//TODO implement
void TranslatorVisitor::visitIfNode(IfNode* node)
{
	WR_ERROR("visitIfNode NI");


	cout << "if ";
    node->ifExpr()->visit(this);
    node->thenBlock()->visit(this);
    if (node->elseBlock())
    {
        cout << "else";
        node->elseBlock()->visit(this);
    }
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode* node)
{
    _code->currentBytecode()->addInsn(BC_ILOAD);
    _code->currentBytecode()->addInt64(node->literal());
    varStack.push(VT_INT);

    WR_DEBUG("visitIntLiteralNode: OK");
}


//TODO implement
void TranslatorVisitor::visitNativeCallNode(NativeCallNode* node)
{
	WR_ERROR("visitNativeCall NI");
	cout << "native call node";
}


void TranslatorVisitor::visitPrintNode(PrintNode* node)
{
	uint32_t n = node->operands();
    for (uint32_t i = 0; i < n; ++i)
    {
    	node->operandAt(i)->visit(this);
    	VarType typeOnTop = varStack.top();
        switch (typeOnTop)
        {
            case VT_INT:
            	_code->currentBytecode()->addInsn(BC_IPRINT);
            	varStack.pop();
        	    break;
            case VT_DOUBLE:
                _code->currentBytecode()->addInsn(BC_DPRINT);
                varStack.pop();
        	    break;
            case VT_STRING:
                _code->currentBytecode()->addInsn(BC_SPRINT);
                varStack.pop();
        	    break;
            default:
            	WR_ERROR("PRINTING INVALID TYPE");
            	break;
        }
    }
}



void TranslatorVisitor::visitReturnNode(ReturnNode* node)
{    

	if (node->returnExpr())
    {
		//WR_DEBUG("visitReturnNode NI");
		cout << "return ";
        node->returnExpr()->visit(this);
        cout << ";" << endl;    
    }        
    //WR_DEBUG("visitReturnNode: OK");
}



//private
void TranslatorVisitor::storeVar(const AstVar* var) {
    WR_DEBUG("storeVar: START");

	VarType type = var->type();

    // somewhere get varId ???
    VarInfo* varInfo = _code->getVarInfoByName(var->name());

    if (!varInfo) {
    	WR_ERROR("VARIABLE NOT FOUND");
    }




    switch (type) {
        case VT_INT:

        	// TODO cast to INT

        	_code->currentBytecode()->addInsn(BC_STORECTXIVAR);
        	_code->currentBytecode()->addUInt16(varInfo->_context);
        	_code->currentBytecode()->addUInt16(varInfo->_id);

        	varStack.pop();

        	//variable poped

        	WR_DEBUG("storeVar: INT : OK");
        	break;

        case VT_DOUBLE:

        	// TODO cast to DOUBLE

            _code->currentBytecode()->addInsn(BC_STORECTXDVAR);
            _code->currentBytecode()->addUInt16(varInfo->_context);
            _code->currentBytecode()->addUInt16(varInfo->_id);


            varStack.pop();

            WR_DEBUG("storeVar: DOUBLE : OK");
        	break;
        case VT_STRING:

        	// TODO cast to STRING

        	_code->currentBytecode()->addInsn(BC_STORECTXSVAR);
            _code->currentBytecode()->addUInt16(varInfo->_context);
            _code->currentBytecode()->addUInt16(varInfo->_id);

            varStack.pop();

            WR_DEBUG("storeVar: STRING : OK");
        	break;
        default:

    	    WR_ERROR("storeVar: NOT OK");
    	    break;
    }



}


void TranslatorVisitor::visitStoreNode(StoreNode* node)
{
	//WR_DEBUG("visitStoreNode: START");

    TokenKind op = node->op();
    const AstVar* var = node->var();

    // TODO check types!
 	//VarType typeOnLeft = node->var()->type();
    //VarType typeOnRight =

    //VarInfo* info = _code->getVarInfoByName(node->var()->name());


    node->value()->visit(this);



    //TODO implements INCRSET and DECRSET

    switch (op) {
        case tINCRSET:
            WR_ERROR("NOT IMPLEMENTED");
        	break;
        case tDECRSET:
        	WR_ERROR("NOT IMPLEMENTED");
        	break;
        case tASSIGN:
            storeVar(var);
            break;
        default:
        	WR_ERROR("Bad storage operation");
            break;
    }
}


void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode* node)
{
	uint16_t stringId = _code->makeStringConstant(node->literal());
	/*
	if (str.size() > 255)
    	WR_ERROR("DON'T SUPPORT SUCH HUGE STRINGS");

    const char* s = str.c_str();
    const uint8_t len = str.size();

    _code->currentBytecode()->addInsn(BC_SLOAD);
    _code->currentBytecode()->addByte(len);


    for (uint8_t i = 0; i < len; ++i)
        _code->currentBytecode()->addByte(s[i]);

    */



	_code->currentBytecode()->addInsn(BC_SLOAD);
	_code->currentBytecode()->addUInt16(stringId);
    varStack.push(VT_STRING);

    WR_DEBUG("visitStringLiteralNode: OK");
}


void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode* node)
{
	WR_ERROR("visitUnaryOpNode NI");

	cout << kindRepr(node->kind());
    node->visitChildren(this);
}


void TranslatorVisitor::visitWhileNode(WhileNode* node)
{
	WR_ERROR("visitWhileNode NI");

	cout << endl <<"while (";
    node->whileExpr()->visit(this);
    cout << ")";
    node->loopBlock()->visit(this);
}



void TranslatorVisitor::visitScopeVars(Scope* scope)
{
    Scope::VarIterator var_iter(scope);

	while (var_iter.hasNext())
    {
        AstVar* curv = var_iter.next();
        VarInfo* info = _code->declareVarInCurrentScope(curv); //, new VarInfo(context, id++));

        cout << "Declared scope var: " << typeRepr(curv->type()) << " " << curv->name() << endl;
        cout << "     " << "CTX: " << info->_context << " ID: " << info->_id << endl;
    }    
}


void TranslatorVisitor::visitScopeFuns(Scope* scope)
{
    WR_DEBUG("");
	WR_DEBUG("visitScopeFuns: START");

	Scope::FunctionIterator fun_iter(scope);
    while (fun_iter.hasNext())
    {
        AstFunction* curf = fun_iter.next();
        visitFunctionNode(curf->node());
    }    
}


void TranslatorVisitor::visitScopeAttr(Scope* scope)
{
    WR_DEBUG("visitScopeAttr");
	visitScopeFuns(scope);
    visitScopeVars(scope);
}





void WR_ERROR(const char* msg) {
    cout << "ERROR: " << msg << endl;
    exit(EXIT_FAILURE);
}


void WR_DEBUG(const char* msg) {
	if (DEBUG_MODE) {
        cout << msg << endl;
	}
}
