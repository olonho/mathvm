#include "Ast2BytecodeVisitor.h"
#include <functional>

namespace mathvm
{

Ast2BytecodeVisitor::Ast2BytecodeVisitor(Code* code)
: _code(code)
, _bytecode(0)
, _bcHelper(_code)
{}

void Ast2BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    // _out << "(";
    // node->left()->visit(this);
    // _out << " " << tokenOp(node->kind()) << " ";
    // node->right()->visit(this);
    // _out << ")";
}
void Ast2BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    if (node->kind() == tSUB ) {
        if (node->operand()->isIntLiteralNode()) {
            int64_t value = - ((IntLiteralNode*) node->operand())->literal();
            bc().load(value);
            return;
        } else if (node->operand()->isDoubleLiteralNode()) {
            double value = - ((DoubleLiteralNode*) node->operand())->literal();
            bc().load(value);
            return;
        }
    } else if (node->kind() == tNOT) {
        // convert_to_logic(source_type);
        // load_const((int64_t)0);
        // do_comparision(tEQ, VT_INT);
    } else {
        node->operand()->visit(this);    
        if (node->kind() == tSUB)
        {
            // if (source_type == VT_INT)
                bc().ineg();
            // else 
                // bc().dneg();
        }
    }
}

void Ast2BytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    bc().load(node->literal());
}
void Ast2BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bc().load(node->literal());
}
void Ast2BytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    bc().load(node->literal());
}

void Ast2BytecodeVisitor::visitLoadNode(LoadNode* node) {
    // bc().load(node->var());
}
void Ast2BytecodeVisitor::visitStoreNode(StoreNode* node) {
    // _out << node->var()->name() << " " << tokenOp(node->op()) << " ";
    // node->visitChildren(this);
    // bc().storevar(node->var(), type)
}

void Ast2BytecodeVisitor::initScope(Scope* scope) {
    // std::string indent(0, ' ');

    // Scope::VarIterator varIt(scope);
    // while (varIt.hasNext()) {
    //     // AstVar* var = varIt.next();
    //     varIt.next();
    //     // _out << indent << mathvm::typeToName(var->type()) << " "<< var->name() << ";" << std::endl;
    // }

    Scope::FunctionIterator funcIt(scope);
    while (funcIt.hasNext()) {
        AstFunction* func = funcIt.next();

        // BytecodeFunction *bytecodeFunction = new BytecodeFunction(func);
        // _code->addFunction(bytecodeFunction);
        // _bytecode = bytecodeFunction->bytecode();

        func->node()->visit(this);

        // _functions.insert(make_pair(fun, bytecode_function));
    }
}

void Ast2BytecodeVisitor::start(AstFunction* top) {
    BytecodeFunction *bytecodeFunction = new BytecodeFunction(top);
    _code->addFunction(bytecodeFunction);
    _bytecode = bytecodeFunction->bytecode();

    top->node()->visit(this);
}

void Ast2BytecodeVisitor::printBlock(BlockNode* node) {
    initScope(node->scope());

    // std::string indent(_indent, ' ');
    for (uint32_t i = 0; i != node->nodes(); ++i)
    {
        // _out << indent;
        node->nodeAt(i)->visit(this);
        if (!node->nodeAt(i)->isIfNode() &&
            !node->nodeAt(i)->isForNode() &&
            !node->nodeAt(i)->isWhileNode() && 
            !node->nodeAt(i)->isBlockNode());
            // _out << ";";
        // _out << std::endl;
    }
}

void Ast2BytecodeVisitor::visitBlockNode(BlockNode* node) {
    // _out << "{" << std::endl;
    // _indent += _indentSize;
    printBlock(node);
    // _indent -= _indentSize;
    // _out << std::string(_indent, ' ') << "}";// << std::endl;
}

void Ast2BytecodeVisitor::visitForNode(ForNode* node) {
    // _out << "for (" 
    //      << node->var()->name()
    //      << " in ";
    // node->inExpr()->visit(this);
    // _out << ") ";
    // node->body()->visit(this);
}

void Ast2BytecodeVisitor::visitWhileNode(WhileNode* node) {
    // _out << "while (";
    // node->whileExpr()->visit(this);
    // _out << ") ";
    // node->loopBlock()->visit(this);
}

void Ast2BytecodeVisitor::visitIfNode(IfNode* node) {
    // _out << "if (";
    // node->ifExpr()->visit(this);
    // _out << ") ";
    // node->thenBlock()->visit(this);
    // if (node->elseBlock()) {
    //     _out << " else ";
    //     node->elseBlock()->visit(this);
    // }
}

void printSignatureElement(std::ostream* out, SignatureElement el) {
    // (*out) << mathvm::typeToName(el.first) << " " << el.second;
}

void Ast2BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
    // _out << "function "
    //      << mathvm::typeToName(node->returnType()) << " ";

    // const Signature& signature = node->signature();
    // printSignature(node->name(), signature.size(),
    //     std::bind1st(std::mem_fun<const SignatureElement&, Signature, size_t>(&Signature::operator[]), &signature),
    //     std::bind1st(std::ptr_fun<std::ostream*, SignatureElement, void>(&printSignatureElement), &_out),
    //     1);

    // if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
    //     node->body()->nodeAt(0)->visit(this);
    // else
       node->visitChildren(this);
}

void Ast2BytecodeVisitor::visitReturnNode(ReturnNode* node) {
    // VarType returnType = get_type(node);
    // if (returnType != VT_VOID)
    // {
    //     node->returnExpr()->visit(this);
    //     convert_to(get_type(node->returnExpr()), returnType);
    // }
    bc().ret();
}

void Ast2BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
    // _out << "native '" << node->nativeName() << "';";
}

template <typename TF1, typename TF2>
void Ast2BytecodeVisitor::printSignature(const std::string& name, uint32_t size, TF1 at, TF2 action, uint32_t begin) const {
    // uint32_t i = begin;
    // _out << name << "(";
    // if (i < size) {
    //     action(at(i++));
    // }
    // while (i < size) {
    //     _out << ", ";
    //     action(at(i++));
    // }
    // _out << ")";
}

void Ast2BytecodeVisitor::visitCallNode(CallNode* node) {
    // printSignature(node->name(), node->parametersNumber(),
    //     std::bind1st(std::mem_fun(&CallNode::parameterAt), node),
    //     std::bind2nd(std::mem_fun(&AstNode::visit), this));
}

void Ast2BytecodeVisitor::visitPrintNode(PrintNode* node) {
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        bc().print();
    }
}
    
}
