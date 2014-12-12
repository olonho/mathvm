#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include <stack>
#include <sstream>

namespace mathvm {

class AstPrinter : public AstVisitor {
    template<class T>
    std::string to_string(T i)
    {
        std::stringstream ss;
        std::string s;
        ss << i;
        s = ss.str();
        
        return s;
    }
    void printScopeDeclarations(Scope* scope);
    void printFunction(AstFunction *function);
    void printBlock(BlockNode *block);
    std::string stringWithEscape(const std::string& str);
    void print(const std::string& str) const {
        std::cout << str;
    }
    void print(const char* str) const {
        std::cout << str;
    }
public:
    virtual ~AstPrinter() {}
    void printTopFunction(AstFunction *top);
	
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);
    
};

class PrintTranslator : public Translator {
public:
    virtual Status* translate(const string& program, Code* *code) {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status && status->isError()) return status;
        AstPrinter printer;
        printer.printTopFunction(parser.top());
        return Status::Ok();
    }
};

void AstPrinter::visitBinaryOpNode(BinaryOpNode* node) {
    print("(");
	node->left()->visit(this);
    print(" ");
	print(tokenOp(node->kind()));
    print(" ");
    node->right()->visit(this);
    print(")");
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
    print("(");
	print(tokenOp(node->kind()));
    node->operand()->visit(this);
    print(")");
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode* node) {
    print("\'");
    print(stringWithEscape(node->literal()));
    print("\'");
}

std::string AstPrinter::stringWithEscape(const std::string& str) {
    std::string strLiteral;
    for(uint32_t i = 0; i < str.size(); ++i) {
        switch(str[i]) {
            case '\'':
                strLiteral += "\\'"; break;
            case '\"':
                strLiteral += "\\\""; break;
            case '\?':
                strLiteral += "\\?"; break;
            case '\\':
                strLiteral += "\\\\"; break;
            case '\a':
                strLiteral += "\\a"; break;
            case '\b':
                strLiteral += "\\b"; break;
            case '\f':
                strLiteral += "\\f"; break;
            case '\n':
                strLiteral += "\\n"; break;
            case '\r':
                strLiteral += "\\r"; break;
            case '\t':
                strLiteral += "\\t"; break;
            case '\v':
                strLiteral += "\\v"; break;
            default:
                strLiteral += str[i];
        }
    }
	return strLiteral;
    
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	print(to_string(node->literal()));
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode* node) {
	print(to_string(node->literal()));
}

void AstPrinter::visitLoadNode(LoadNode* node) {
	print(node->var()->name());
}

void AstPrinter::visitStoreNode(StoreNode* node) {
    print(node->var()->name());
    print(" ");
    print(tokenOp(node->op()));
    print(" ");
    node->value()->visit(this);
}

void AstPrinter::visitForNode(ForNode* node) {
	print("for (");
    print(node->var()->name());
    print(" in ");
    node->inExpr()->visit(this);
    print(" ) ");
    node->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode* node) {
	print("while (");
    node->whileExpr()->visit(this);
    print(") ");
    node->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode(IfNode* node) {
	print ("if (");
    node->ifExpr()->visit(this);
    print(")" );
    node->thenBlock()->visit(this);
    AstNode* elseBlock = node->elseBlock();
    if (elseBlock) {
        print(" else ");
        elseBlock->visit(this);
    }
}

void AstPrinter::printBlock(BlockNode *block) {
    printScopeDeclarations(block->scope());
    for (uint32_t i = 0; i < block->nodes(); i++) {
        block->nodeAt(i)->visit(this);
        print(";\n");
    }
}

void AstPrinter::visitBlockNode(BlockNode* node) {
    print("{\n");
	printBlock(node);
    print("}\n");
}

void AstPrinter::visitFunctionNode(FunctionNode* node) {
	assert(false);
}

void AstPrinter::visitReturnNode(ReturnNode* node) {
	print ("return");
    if (node->returnExpr()) {
        print(" ");
    	node->returnExpr()->visit(this);
    }
}

void AstPrinter::visitCallNode(CallNode* node) {
	print(node->name());
    print("(");
	for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i != 0)
            print(", ");
        node->parameterAt(i)->visit(this);
    }
    print(")");
}

void AstPrinter::visitNativeCallNode(NativeCallNode* node) {
	print("native \'");
    print(node->nativeName());
    print("\';");
}

void AstPrinter::visitPrintNode(PrintNode* node) {
	print("print(");
    for (uint32_t i = 0; i < node->operands(); i++) {
        if (i != 0)
            print(", ");
        node->operandAt(i)->visit(this);
    }
    print(")");
}

void AstPrinter::printScopeDeclarations(Scope* scope) {
    Scope::VarIterator varIterator(scope);
    while(varIterator.hasNext()) {
        AstVar *var = varIterator.next();
        print(typeToName(var->type()));
        print(" ");
        print(var->name());
        print(";\n");
    }
    
    Scope::FunctionIterator funcIterator(scope);
    while(funcIterator.hasNext()) {
        AstFunction *func = funcIterator.next();
		printFunction(func);
    }
    print("\n");
}

void AstPrinter::printFunction(AstFunction *function) {
    print("function ");
    print(typeToName(function->returnType()));
    print(" ");
    print(function->name());
    print("(");
    for(uint32_t i  = 0; i < function->parametersNumber(); i++) {
        if (i != 0)
            print(", ");
        print(typeToName(function->parameterType(i)));
        print(" ");
        print(function->parameterName(i));
    }
    print(") ");
    BlockNode *block = function->node()->body();
    if (block->nodeAt(0) && block->nodeAt(0)->isNativeCallNode()) {
        block->nodeAt(0)->visit(this);
        print("\n");
    } else {
		function->node()->body()->visit(this);
    }
}

void AstPrinter::printTopFunction(AstFunction *top) {
	printBlock(top->node()->body());
}
    

    
Translator* Translator::create(const string& impl) {
	if (impl == "printer") {
		return new PrintTranslator();
	} else {
		return NULL;
	}
}

    
}
