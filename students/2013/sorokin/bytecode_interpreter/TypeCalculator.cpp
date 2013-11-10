#include "TypeCalculator.h"


namespace mathvm {
    
    
    TypeCalculator::TypeCalculator(AstTypeMap& typeMap, ostream& errorStream)
        : myMap(typeMap), myErr(errorStream)  {}

    TypeCalculator::~TypeCalculator() {
        myErr.flush();
    }
    
    void TypeCalculator::calculateTypes(Parser& parser) {
        if(parser.top() == 0) { 
            myErr << "parser.top() equals null" << endl;
            return;
        }
        
        this->visitAstFunction(parser.top());
    }

    void TypeCalculator::visitAstFunction(AstFunction* function) {
        Scope::VarIterator varIter(function->scope());     

        while(varIter.hasNext()) {             
            AstVar* var = varIter.next();
            myMap[var] = var->type(); 
        }

        function->node()->visit(this);
    }
    
    void TypeCalculator::visitBinaryOpNode(BinaryOpNode* node) {
        AstNode* leftNode = node->left();
        AstNode* rightNode = node->right();
        leftNode->visit(this);
        rightNode->visit(this);
        
        myMap[node] = getBinaryOpType(myMap[leftNode], myMap[rightNode], node->kind());
    }
    
    VarType TypeCalculator::getBinaryOpType(VarType typeA, VarType typeB, TokenKind kind) {
        
        if(typeA != VT_INT && typeA != VT_DOUBLE) return VT_INVALID;
        if(typeB != VT_INT && typeB != VT_DOUBLE) return VT_INVALID;
        
        switch (kind) {
            case tOR: 
            case tAND: 
            case tEQ:
            case tNEQ: 
            case tGT:
            case tGE:
            case tLT:
            case tLE:    
            case tMOD: return VT_INT;
            case tDIV: 
            case tADD:
            case tSUB:
            case tMUL: return (typeA != typeB ? VT_DOUBLE : typeA); 
            default:   return VT_INVALID;
        } 
    }

    void TypeCalculator::visitIntLiteralNode(IntLiteralNode* node) {
        myMap[node] = VT_INT;
    }

    void TypeCalculator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        myMap[node] = VT_DOUBLE;
    }
    
    void TypeCalculator::visitStringLiteralNode(StringLiteralNode* node) {
        myMap[node] = VT_STRING;
    }
    

    void TypeCalculator::visitUnaryOpNode(UnaryOpNode* node) {
        node->visitChildren(this);
        if(myMap[node->operand()] != VT_INT && myMap[node->operand()] != VT_DOUBLE)
            myMap[node] = VT_INVALID;
        else if(node->kind() == tNOT && myMap[node->operand()] != VT_INT)
            myMap[node] = VT_INVALID;
        else
            myMap[node] = myMap[node->operand()];
    }

    void TypeCalculator::visitCallNode(CallNode* node) {
            node->visitChildren(this);
            AstFunction* function = myScopes.top()->lookupFunction(node->name());
            if(function == 0) {
                myErr << "visitCallNode error:" 
                      << " cant find function: " 
                      << node->name() << endl;
                return;
            }
            
            myMap[node] = function->returnType();
    }

    void TypeCalculator::visitBlockNode(BlockNode* node) {
            myScopes.push(node->scope());
            
            Scope::FunctionIterator fIter(node->scope());
            while(fIter.hasNext())
                visitAstFunction(fIter.next());
            
            node->visitChildren(this);
            
            myScopes.pop();
    }

    void TypeCalculator::visitLoadNode(LoadNode* node) {
            myMap[node] = node->var()->type();
    }


    void TypeCalculator::visitStoreNode(StoreNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitForNode(ForNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitWhileNode(WhileNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitIfNode(IfNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitFunctionNode(FunctionNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitReturnNode(ReturnNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitPrintNode(PrintNode* node) {
            node->visitChildren(this);
    }

    void TypeCalculator::visitNativeCallNode(NativeCallNode* node) { }

}