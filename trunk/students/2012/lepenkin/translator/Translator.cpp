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

TranslatorVisitor::~TranslatorVisitor()
{
}




void TranslatorVisitor::translate(AstFunction* top)
{
	//cout << "Bytecode:" << endl << endl;

    _code->pushScope();
    _code->declareFunctionInCurrentScope(top->node());

    top->node()->visit(this);


    ((BytecodeFunction*)_code->functionById(0))->bytecode()->addInsn(BC_STOP);
    //_code->currentBytecode()->addInsn(BC_STOP);

    //cout << endl << "Translated." << endl;
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

void TranslatorVisitor::ifCondLoad1Else0()
{
	_code->currentBytecode()->addInt16(4);

    _code->currentBytecode()->addInsn(BC_ILOAD0);
    _code->currentBytecode()->addInsn(BC_JA);
    _code->currentBytecode()->addInt16(1);

    _code->currentBytecode()->addInsn(BC_ILOAD1);

    varStack.push(VT_INT);
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode* node)
{
    node->left()->visit(this);
    node->right()->visit(this);


    // i guess they pushed on stack

    VarType top = varStack.top();
    varStack.pop();
    VarType down = varStack.top();
    varStack.pop();

    VarType widest = (top == VT_DOUBLE || down == VT_DOUBLE) ? VT_DOUBLE : VT_INT;

    //cout << "BINARY POPPED 2X. varStack: " << varStack.size() << endl;


    switch (node->kind()) {


        case tAND:
            _code->currentBytecode()->addInsn(BC_IADD);
            varStack.push(VT_INT);
            _code->currentBytecode()->addInsn(BC_ILOAD1);
            _code->currentBytecode()->addInsn(BC_IFICMPG);
            varStack.pop();

            ifCondLoad1Else0();
        	break;

        case tOR:
        	_code->currentBytecode()->addInsn(BC_IADD);
        	varStack.push(VT_INT);

        	_code->currentBytecode()->addInsn(BC_ILOAD1);
        	_code->currentBytecode()->addInsn(BC_IFICMPGE);
        	varStack.pop();

        	ifCondLoad1Else0();
        	break;

        case tSUB:
        	//TODO type check and conversion
        	_code->currentBytecode()->addInsn(BC_SWAP);
        	switch (widest)
        	{
        	    case VT_DOUBLE:
        	    	_code->currentBytecode()->addInsn(BC_DSUB);
        	    	varStack.push(VT_DOUBLE);
        	    	break;
        	    case VT_INT:
        	    	_code->currentBytecode()->addInsn(BC_ISUB);
        	    	varStack.push(VT_INT);
        	    	break;
       	        default:
        	        assert(false);
       	        	break;
        	}
            break;

        case tADD:
        	switch (widest)
        	{
        	    case VT_DOUBLE:
        	    	_code->currentBytecode()->addInsn(BC_DADD);
        	     	varStack.push(VT_DOUBLE);
        	    	break;
        	    case VT_INT:
        	        _code->currentBytecode()->addInsn(BC_IADD);
        	        varStack.push(VT_INT);
        	        break;
        	    default:
        	        assert(false);
        	    	break;
        	 }
        	break;
        case tMUL:
        	switch (widest)
        	{
        	    case VT_DOUBLE:
        	    	_code->currentBytecode()->addInsn(BC_DMUL);
        	     	varStack.push(VT_DOUBLE);
        	    	break;
        	    case VT_INT:
        	        _code->currentBytecode()->addInsn(BC_IMUL);
        	     	varStack.push(VT_INT);
        	        break;
        	    default:
        	        assert(false);
        	    	break;
        	}
        	break;

        case tDIV:
        	_code->currentBytecode()->addInsn(getDivInsn(widest));
        	varStack.push(widest);
        	break;


        case tEQ:
        	switch (widest) {
        	    case VT_INT:
        		    _code->currentBytecode()->addInsn(BC_ICMP);
        		    break;
        	    case VT_DOUBLE:
        	    	_code->currentBytecode()->addInsn(BC_DCMP);
        	    	break;
        	    default:
        	    	assert(false);
        	    	break;
        	}
        	varStack.push(VT_INT);

            _code->currentBytecode()->addInsn(BC_ILOAD0);
            _code->currentBytecode()->addInsn(BC_IFICMPE);
            varStack.pop();

            ifCondLoad1Else0();
            break;

        case tGT:
        	switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    break;
                case VT_DOUBLE:
                   	_code->currentBytecode()->addInsn(BC_DCMP);
                   	 break;
                default:
                   	assert(false);
                  	break;
            }
        	varStack.push(VT_INT);

             _code->currentBytecode()->addInsn(BC_ILOAD1);
             _code->currentBytecode()->addInsn(BC_IFICMPE);
             varStack.pop();

             ifCondLoad1Else0();
             break;

        case tLT:
        	switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    varStack.push(VT_INT);
                    break;
                case VT_DOUBLE:
                  	_code->currentBytecode()->addInsn(BC_DCMP);
                    varStack.push(VT_DOUBLE);
                  	break;
                default:
                   	assert(false);
                   	break;
            }
        	varStack.push(VT_INT);
             _code->currentBytecode()->addInsn(BC_ILOADM1);
             _code->currentBytecode()->addInsn(BC_IFICMPE);
             varStack.pop();
             ifCondLoad1Else0();
             break;

        case tLE:
            switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    break;
                case VT_DOUBLE:
                  	_code->currentBytecode()->addInsn(BC_DCMP);
                    break;
                default:
                   	assert(false);
                   	break;
            }
            varStack.push(VT_INT);
             _code->currentBytecode()->addInsn(BC_ILOAD0);
             _code->currentBytecode()->addInsn(BC_IFICMPLE);
             varStack.pop();

             ifCondLoad1Else0();
             break;

        case tGE:
        	switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    break;
                case VT_DOUBLE:
                  	_code->currentBytecode()->addInsn(BC_DCMP);
                    break;
                default:
                  	assert(false);
                  	break;
            }
        	varStack.push(VT_INT);
            _code->currentBytecode()->addInsn(BC_ILOAD0);
            _code->currentBytecode()->addInsn(BC_IFICMPGE);
            varStack.pop();
            ifCondLoad1Else0();
            break;

        case tNEQ:
        	switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    break;
                case VT_DOUBLE:
                  	_code->currentBytecode()->addInsn(BC_DCMP);
                    break;
                default:
                   	assert(false);
                   	break;
            }
        	varStack.push(VT_INT);
             _code->currentBytecode()->addInsn(BC_ILOAD0);
             _code->currentBytecode()->addInsn(BC_IFICMPNE);
             varStack.pop();
             ifCondLoad1Else0();
             break;

         case tRANGE:
        	 assert(false);
        	 _code->currentBytecode()->addInsn(BC_SWAP);
        	 break;


/*
        case tLE:
            switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    break;
                case VT_DOUBLE:
                  	_code->currentBytecode()->addInsn(BC_DCMP);
                    break;
                default:
                   	assert(false);
                   	break;
             }

             _code->currentBytecode()->addInsn(BC_ILOAD0);
             _code->currentBytecode()->addInsn(BC_IFICMPLE);
             ifCondLoad1Else0();
             break;

        case tLE:
            switch (widest) {
                case VT_INT:
                    _code->currentBytecode()->addInsn(BC_ICMP);
                    break;
                case VT_DOUBLE:
                  	_code->currentBytecode()->addInsn(BC_DCMP);
                    break;
                default:
                   	assert(false);
                   	break;
             }

             _code->currentBytecode()->addInsn(BC_ILOAD0);
             _code->currentBytecode()->addInsn(BC_IFICMPLE);
             ifCondLoad1Else0();
             break;

 */


        default:
        	//cout << "IN BIN_OPS SMTH NOT IMPLEMENTED" << endl;
        	assert(false);
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
    //WR_DEBUG("visitBlockNode: START");
    //cout << "Scope: " << node->scope() << endl;
	visitScopeVars(node->scope());
	visitScopeFuns(node->scope());


	_code->pushScope();
    visitBlockNodeBody(node);
    _code->popScope();

    //WR_DEBUG("visitBlockNode: END");
}

void TranslatorVisitor::visitCallNode(CallNode* node)
{
	//cout << "varStack: " << varStack.size() << endl;
	//cout << "______START_____CALL______NODE______" << endl;


	FunInfo* info = _code->getFunIdByName(node->name());
    assert(info);

    //cout << "BEFORE LOADING PARAMS varStack: " << varStack.size() << endl;
    uint32_t n = node->parametersNumber();
    for (int i = n - 1; i >= 0; --i)
    {
      	//cout << "fun param pushed ?" << i << endl;
       	node->parameterAt(i)->visit(this);
        varStack.pop();
    }
    //cout << "AFTER LOADING PARAMS varStack: " << varStack.size() << endl;

    _code->currentBytecode()->addInsn(BC_CALL);
    _code->currentBytecode()->addUInt16(info->_id);

    varStack.push(_code->functionById(info->_id)->returnType());

    //cout << "______END_____CALL______NODE______" << endl;
    //cout << "varStack: " << varStack.size() << endl;
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
	_code->currentBytecode()->addInsn(BC_DLOAD);
    _code->currentBytecode()->addDouble(node->literal());
    varStack.push(VT_DOUBLE);

    //cout << "BC_DLOAD " << node->literal() << " varStack: " << varStack.size() << endl;
}

void TranslatorVisitor::visitForNode(ForNode* node)
{

	const AstVar* var = node->var();

    node->inExpr()->asBinaryOpNode()->left()->visit(this);
    varStack.push(var->type());
    storeVar(var);

    uint32_t check = _code->currentBytecode()->current();
    loadVar(var);
    node->inExpr()->asBinaryOpNode()->right()->visit(this);
    varStack.push(var->type());
    _code->currentBytecode()->addInsn(BC_IFICMPG);
    varStack.pop();
    varStack.pop();
    uint32_t jumpToEnd = _code->currentBytecode()->current();
    _code->currentBytecode()->addInt16(0);
    node->body()->visit(this);
    loadVar(var);
    _code->currentBytecode()->addInsn(getLoad1Insn(var->type()));
    _code->currentBytecode()->addInsn(getPlusInsn(var->type()));
    storeVar(var);
    _code->currentBytecode()->addInsn(BC_JA);
    uint32_t jumpToStart = _code->currentBytecode()->current();
    _code->currentBytecode()->addInt16(0);
    uint32_t end = _code->currentBytecode()->current();
    _code->currentBytecode()->setInt16(jumpToStart, check - end);
    _code->currentBytecode()->setInt16(jumpToEnd, end - jumpToEnd - 2);

}

void TranslatorVisitor::visitFunctionNode(FunctionNode* node)
{
    //WR_DEBUG("visitFunctionNode: START");

	//BytecodeFunction* fun =a _code->declareFunctionInCurrentScope(node);

    //cout << "Added bytecode function. ID: " << fun->id() << " Name: " << fun->name() << endl;
    //cout << "Searching fun by name: " << _code->getFunIdByName(fun->name())->_id << endl;


	FunInfo* funInfo = _code->getFunIdByName(node->name());
	assert(funInfo);

	BytecodeFunction* fun = (BytecodeFunction*) _code->functionById(funInfo->_id);

    _code->pushFunction(fun);
    _code->pushScope();


    uint32_t params = node->parametersNumber();
    for (uint32_t i = 0; i < params; ++i) {
    	string name = node->parameterName(i);
        VarType type = node->parameterType(i);
    	//VarInfo* info =
        _code->declareVarInCurrentScope(new AstVar(name, type, 0));
        //cout << "Declared function param: " << typeRepr(type) << " " << name << endl;
        //cout << "               CTX: " << info->_context << " ID: " << info->_id << endl;
     }

     // was visitScopeVars and Funs

     node->body()->visit(this);

    _code->popFunction();
    _code->popScope();

   // WR_DEBUG("visitFunctionNode: OK");
}


// int x; ?

// load on TOS ?
void TranslatorVisitor::visitLoadNode(LoadNode* node)
{
	const AstVar* var = node->var();
	VarInfo* info = _code->getVarInfoByName(var->name());
    assert(info);

	Instruction ins;
	switch (var->type()) {
	    case VT_INT:
		    ins = BC_LOADCTXIVAR;
		    //cout << "LOADCTX INT ";
		    break;
	    case VT_DOUBLE:
	    	ins = BC_LOADCTXDVAR;
	    	//cout << "LOADCTX DOUBLE ";
	    	break;
	    case VT_STRING:
	    	ins = BC_LOADCTXSVAR;
	    	//cout << "LOADCTX STRING ";
	        break;
	    default:
	    	assert(false);
	    	break;
	}

	//TODO UInt16
    _code->currentBytecode()->addInsn(ins);
    _code->currentBytecode()->addInt16(info->_context);
    _code->currentBytecode()->addInt16(info->_id);



    varStack.push(var->type());

    //cout << "varStack: " << varStack.size() << endl;
}



//TODO implement
void TranslatorVisitor::visitIfNode(IfNode* node)
{

	node->ifExpr()->visit(this);

	// on stack result of comparison
	_code->currentBytecode()->addInsn(BC_ILOAD0);
	varStack.push(VT_INT);
	_code->currentBytecode()->addInsn(BC_IFICMPE);
    varStack.pop();
    varStack.pop();

	uint32_t elsePointer = _code->currentBytecode()->current();
	_code->currentBytecode()->addInt16(0);
    uint32_t thenStartPtr = _code->currentBytecode()->current();


	node->thenBlock()->visit(this);
    _code->currentBytecode()->addInsn(BC_JA);
    uint32_t endPointer = _code->currentBytecode()->current();
    _code->currentBytecode()->addInt16(0);
    uint32_t elseStartPtr = _code->currentBytecode()->current();


    _code->currentBytecode()->setInt16(elsePointer, elseStartPtr - thenStartPtr);

	if (node->elseBlock())
	{
	    node->elseBlock()->visit(this);
	}

    uint32_t endOfIfBlock = _code->currentBytecode()->current();
    _code->currentBytecode()->setInt16(endPointer, endOfIfBlock - elseStartPtr);
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode* node)
{
    _code->currentBytecode()->addInsn(BC_ILOAD);
    _code->currentBytecode()->addInt64(node->literal());
    varStack.push(VT_INT);

    //cout << "BC_ILOAD " << node->literal() << " varStack: " << varStack.size() << endl;
}


//TODO implement
void TranslatorVisitor::visitNativeCallNode(NativeCallNode* node)
{
	assert(false);
}


void TranslatorVisitor::visitPrintNode(PrintNode* node)
{
	uint32_t n = node->operands();
    for (uint32_t i = 0; i < n; ++i)
    {
    	node->operandAt(i)->visit(this);
    	//cout << "PRINT PARAM " << i << endl;
    	assert(varStack.size() != 0);
    	VarType typeOnTop = varStack.top();

    	switch (typeOnTop)
        {
            case VT_INT:
            	_code->currentBytecode()->addInsn(BC_IPRINT);
            	//cout << "IPRINT" << endl;

        	    break;
            case VT_DOUBLE:
                _code->currentBytecode()->addInsn(BC_DPRINT);
                //cout << "DPRINT" << endl;

        	    break;
            case VT_STRING:
                _code->currentBytecode()->addInsn(BC_SPRINT);
                //cout << "SPRINT" << endl;
        	    break;
            default:
            	assert(false);
            	break;
        }
        varStack.pop();
        //cout << "_bc->length " << _code->currentBytecode()->length() << endl;

    }
}



void TranslatorVisitor::visitReturnNode(ReturnNode* node)
{    
	if (node->returnExpr())
    {
		node->returnExpr()->visit(this);
        assert(varStack.size() != 0);
		varStack.pop();
    }
	_code->currentBytecode()->addInsn(BC_RETURN);
	//cout << "RETURN PUSHED" << endl;
}



//private
void TranslatorVisitor::storeVar(const AstVar* var) {
	VarType type = var->type();
    VarInfo* varInfo = _code->getVarInfoByName(var->name());
    if (!varInfo) {
    	//cout << "variable not found" << endl;
    	assert(false);
    }
    //cout << "STORED ";
    switch (type) {
        case VT_INT:
        	// TODO cast to INT
        	_code->currentBytecode()->addInsn(BC_STORECTXIVAR);
        	_code->currentBytecode()->addUInt16(varInfo->_context);
        	_code->currentBytecode()->addUInt16(varInfo->_id);
        	//cout << "INT ";
        	break;
        case VT_DOUBLE:
            _code->currentBytecode()->addInsn(BC_STORECTXDVAR);
            _code->currentBytecode()->addUInt16(varInfo->_context);
            _code->currentBytecode()->addUInt16(varInfo->_id);
            //cout << "DOUBLE ";
        	break;
        case VT_STRING:
        	_code->currentBytecode()->addInsn(BC_STORECTXSVAR);
            _code->currentBytecode()->addUInt16(varInfo->_context);
            _code->currentBytecode()->addUInt16(varInfo->_id);
            //cout << "STRING ";
        	break;
        default:
        	assert(false);
        	break;
    }

    varStack.pop();
    //cout << "POPPED. varStack: " << varStack.size() << endl;
}

void TranslatorVisitor::loadVar(const AstVar* var) {
	VarType type = var->type();
    VarInfo* varInfo = _code->getVarInfoByName(var->name());
    assert(varInfo);
    //cout << "LOADED ";
    switch (type) {
        case VT_INT:
        	// TODO cast to INT
        	_code->currentBytecode()->addInsn(BC_LOADCTXIVAR);
        	_code->currentBytecode()->addUInt16(varInfo->_context);
        	_code->currentBytecode()->addUInt16(varInfo->_id);
        	//cout << "INT ";
        	break;
        case VT_DOUBLE:
            _code->currentBytecode()->addInsn(BC_LOADCTXDVAR);
            _code->currentBytecode()->addUInt16(varInfo->_context);
            _code->currentBytecode()->addUInt16(varInfo->_id);
            //cout << "DOUBLE ";
        	break;
        case VT_STRING:
        	_code->currentBytecode()->addInsn(BC_LOADCTXSVAR);
            _code->currentBytecode()->addUInt16(varInfo->_context);
            _code->currentBytecode()->addUInt16(varInfo->_id);
            //cout << "STRING ";
        	break;
        default:
        	assert(false);
        	break;
    }
    varStack.push(type);
    //cout << "PUSHED. varStack: " << varStack.size() << endl;
}





void TranslatorVisitor::visitStoreNode(StoreNode* node)
{
	TokenKind op = node->op();
    const AstVar* var = node->var();


    // TODO check types!
    node->value()->visit(this);
    switch (op) {
        case tINCRSET:
        	loadVar(var);
        	_code->currentBytecode()->addInsn(getPlusInsn(var->type()));
        	varStack.pop();
        	storeVar(var);
        	break;
        case tDECRSET:
        	loadVar(var);
        	_code->currentBytecode()->addInsn(getMinusInsn(var->type()));
        	storeVar(var);
        	break;
        case tASSIGN:
            storeVar(var);
            break;
        default:
        	assert(false);
        	break;
    }
}


void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode* node)
{
	uint16_t stringId = _code->makeStringConstant(node->literal());
	_code->currentBytecode()->addInsn(BC_SLOAD);
	_code->currentBytecode()->addUInt16(stringId);
    varStack.push(VT_STRING);

    //cout << "BC_SLOAD " << node->literal() << " varStack: " << varStack.size() << endl;
}


void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode* node)
{
	node->visitChildren(this);
	switch (node->kind()) {
	    case tNOT:
	    	_code->currentBytecode()->addInsn(BC_ILOAD0);
            _code->currentBytecode()->addInsn(BC_IFICMPE);
            varStack.pop();
            ifCondLoad1Else0();
            varStack.push(VT_INT);
		    break;
	    case tSUB:
	    	_code->currentBytecode()->addInsn(getNegInsn(varStack.top()));
	    	break;
	    default:
		    assert(false);
		    break;
	}
}


void TranslatorVisitor::visitWhileNode(WhileNode* node)
{

	uint32_t start = _code->currentBytecode()->current();
	node->whileExpr()->visit(this);

	_code->currentBytecode()->addInsn(BC_ILOAD0);
	varStack.push(VT_INT);
	_code->currentBytecode()->addInsn(BC_IFICMPE);
    varStack.pop();
    varStack.pop();
    uint32_t jumpToEnd = _code->currentBytecode()->current();
   	_code->currentBytecode()->addInt16(0);
    node->loopBlock()->visit(this);
    _code->currentBytecode()->addInsn(BC_JA);
    uint32_t jumpToStart = _code->currentBytecode()->current();
    _code->currentBytecode()->addInt16(0);
	uint32_t end = _code->currentBytecode()->current();
    _code->currentBytecode()->setInt16(jumpToStart, start - end);

    _code->currentBytecode()->setInt16(jumpToEnd, end - jumpToEnd - 2);

}



void TranslatorVisitor::visitScopeVars(Scope* scope)
{
    Scope::VarIterator var_iter(scope);

	while (var_iter.hasNext())
    {
        AstVar* curv = var_iter.next();
        //VarInfo* info =
        _code->declareVarInCurrentScope(curv);

        //cout << "Declared scope var: " << typeRepr(curv->type()) << " " << curv->name() << endl;
        //cout << "     " << "CTX: " << info->_context << " ID: " << info->_id << endl;
    }    
}


void TranslatorVisitor::visitScopeFuns(Scope* scope)
{
    Scope::FunctionIterator fun_iter(scope);
    Scope::FunctionIterator decl_iter(scope);

    while (fun_iter.hasNext())
    {
        AstFunction* curf = fun_iter.next();
        _code->declareFunctionInCurrentScope(curf->node());
    }

    while (decl_iter.hasNext())
    {
        AstFunction* curf = decl_iter.next();
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
    //cout << "ERROR: " << msg << endl;
    exit(EXIT_FAILURE);
}


void WR_DEBUG(const char* msg) {
	if (DEBUG_MODE) {
        //cout << msg << endl;
	}
}
