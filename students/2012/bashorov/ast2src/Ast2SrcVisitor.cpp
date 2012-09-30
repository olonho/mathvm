#include "Ast2SrcVisitor.h"
#include <functional>

namespace mathvm
{

Ast2SrcVisitor::Ast2SrcVisitor(std::ostream& out, uint32_t indentSize)
: _out(out)
, _indent(0)
, _indentSize(indentSize)
{}

void Ast2SrcVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    _out << "(";
    node->left()->visit(this);
    _out << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    _out << ")";
}
void Ast2SrcVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    _out << tokenOp(node->kind());
    node->operand()->visit(this);
}

std::string escape(const std::string& str) {
    std::string t;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        switch (*it) {
        case '\n':
            t += "\\n";
            break;
        case '\r':
            t +=  "\\r";
            break;
        case '\\':
            t +=  "\\\\";
            break;
        case '\t':
            t +=  "\\t";
            break;
        default:
            t += *it;
        }
    }
    return t;
}

void Ast2SrcVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    _out << "\'" << escape(node->literal()) << "\'";
}
void Ast2SrcVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _out << node->literal();
}
void Ast2SrcVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    _out << node->literal();
}

void Ast2SrcVisitor::visitLoadNode(LoadNode* node) {
    _out << node->var()->name();
}
void Ast2SrcVisitor::visitStoreNode(StoreNode* node) {
    _out << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
}

void Ast2SrcVisitor::initScope(Scope* scope) {
    std::string indent(_indent, ' ');

    Scope::VarIterator varIt(scope);
    while (varIt.hasNext()) {
        AstVar* var = varIt.next();
        _out << indent << mathvm::typeToName(var->type()) << " "<< var->name() << ";" << std::endl;
    }

    Scope::FunctionIterator funcIt(scope);
    while (funcIt.hasNext()) {
        funcIt.next()->node()->visit(this);
        _out << std::endl;
    }
}

void Ast2SrcVisitor::printBlock(BlockNode* node) {
    initScope(node->scope());

    std::string indent(_indent, ' ');
    for (uint32_t i = 0; i != node->nodes(); ++i)
    {
        _out << indent;
        node->nodeAt(i)->visit(this);
        if (!node->nodeAt(i)->isIfNode() &&
            !node->nodeAt(i)->isForNode() &&
            !node->nodeAt(i)->isWhileNode() && 
            !node->nodeAt(i)->isBlockNode())
            _out << ";";
        _out << std::endl;
    }
}

void Ast2SrcVisitor::visitBlockNode(BlockNode* node) {
    _out << "{" << std::endl;
    _indent += _indentSize;
    printBlock(node);
    _indent -= _indentSize;
    _out << std::string(_indent, ' ') << "}";// << std::endl;
}

void Ast2SrcVisitor::visitForNode(ForNode* node) {
    _out << "for (" 
         << node->var()->name()
         << " in ";
    node->inExpr()->visit(this);
    _out << ") ";
    node->body()->visit(this);
}

void Ast2SrcVisitor::visitWhileNode(WhileNode* node) {
    _out << "while (";
    node->whileExpr()->visit(this);
    _out << ") ";
    node->loopBlock()->visit(this);
}

void Ast2SrcVisitor::visitIfNode(IfNode* node) {
    _out << "if (";
    node->ifExpr()->visit(this);
    _out << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        _out << " else ";
        node->elseBlock()->visit(this);
    }
}

void printSignatureElement(std::ostream* out, SignatureElement el) {
    (*out) << mathvm::typeToName(el.first) << " " << el.second;
}

void Ast2SrcVisitor::visitFunctionNode(FunctionNode* node) {
    _out << "function "
         << mathvm::typeToName(node->returnType()) << " ";

    const Signature& signature = node->signature();
    printSignature(node->name(), signature.size(),
        std::bind1st(std::mem_fun<const SignatureElement&, Signature, size_t>(&Signature::operator[]), &signature),
        std::bind1st(std::ptr_fun<std::ostream*, SignatureElement, void>(&printSignatureElement), &_out),
        1);

    if (node->body()->nodes() && node->body()->nodeAt(0)->isNativeCallNode())
        node->body()->nodeAt(0)->visit(this);
    else
        node->visitChildren(this);
}
void Ast2SrcVisitor::visitReturnNode(ReturnNode* node) {
    _out << "return ";
    node->visitChildren(this);
}

void Ast2SrcVisitor::visitNativeCallNode(NativeCallNode* node) {
    _out << "native '" << node->nativeName() << "';";
}

template <typename TF1, typename TF2>
void Ast2SrcVisitor::printSignature(const std::string& name, uint32_t size, TF1 at, TF2 action, uint32_t begin) const {
    uint32_t i = begin;
    _out << name << "(";
    if (i < size) {
        action(at(i++));
    }
    while (i < size) {
        _out << ", ";
        action(at(i++));
    }
    _out << ")";
}

void Ast2SrcVisitor::visitCallNode(CallNode* node) {
    printSignature(node->name(), node->parametersNumber(),
        std::bind1st(std::mem_fun(&CallNode::parameterAt), node),
        std::bind2nd(std::mem_fun(&AstNode::visit), this));
}

void Ast2SrcVisitor::visitPrintNode(PrintNode* node) {
    printSignature("print", node->operands(),
        std::bind1st(std::mem_fun(&PrintNode::operandAt), node),
        std::bind2nd(std::mem_fun(&AstNode::visit), this));
}
    
}
