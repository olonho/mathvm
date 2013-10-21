#include "Translator.h"

#include "CodeImpl.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

namespace mathvm {

class TranslatorVisitor;
Status* BytecodeTranslator::translate(string const & program, Code ** code) {
    Parser parser;
    Status * status = parser.parseProgram(program);
    if (status && status->isError()) {
        return status;
    } else {
        *code = new CodeImpl();
        // parser.top()->node()
        return NULL;
    }

}

class TranslatorVisitor : AstVisitor {
    Code * code;
    Bytecode * bc;
    VarType tos_type;
public:
    TranslatorVisitor(Code * code)
        : code(code), tos_type(VT_INVALID) {}
    virtual ~TranslatorVisitor() {}

    virtual void visitBinaryOpNode(BinaryOpNode * node) {
        Label yes(bc);
        Label end(bc);

        node->right()->visit(this);
        VarType right = tos_type;
        node->left()->visit(this);
        VarType left = tos_type;

        assertSame(right, left);
        assertArithmetic(left);

        switch(node->kind()) {
            case tOR:       //"||"
                assertInt(left);
                addInsn(BC_IAOR);

                tos_type = VT_INT;
                return;
            case tAND:      //"&&"
                assertInt(left);
                addInsn(BC_IMUL);

                tos_type = VT_INT;
                return;
            case tAAND:     //"&"
                assertInt(left);
                addInsn(BC_IAAND);

                tos_type = VT_INT;
                return;
            case tAOR:      //"|"
                assertInt(left);
                addInsn(BC_IAOR);

                tos_type = VT_INT;
                return;
            case tAXOR:     //"^"
                assertInt(left);
                addInsn(BC_IAXOR);

                tos_type = VT_INT;
                return;
            case tEQ:           //"=="
                addInsn(BC_ICMP, left);
                addInsn(BC_ILOAD1);
                addInsn(BC_IAAND);
                addInsn(BC_ILOAD1);
                addInsn(BC_IAXOR);

                tos_type = VT_INT;
                return;
            case tNEQ:          //"!="
                addInsn(BC_ICMP, left);

                tos_type = VT_INT;
                return;
            case tGT:           //">"
                addInsn(BC_ICMP, left);
                addInsn(BC_ILOAD0);

                bc->addBranch(BC_IFICMPL, yes);
                addInsn(BC_ILOAD0);
                bc->addBranch(BC_JA, end);
                bc->bind(yes);
                addInsn(BC_ILOAD1);
                bc->bind(end);

                tos_type = VT_INT;
                return;
            case tGE:           //">="
                addInsn(BC_ICMP, left);
                addInsn(BC_ILOAD0);

                bc->addBranch(BC_IFICMPLE, yes);
                addInsn(BC_ILOAD0);
                bc->addBranch(BC_JA, end);
                bc->bind(yes);
                addInsn(BC_ILOAD1);
                bc->bind(end);

                tos_type = VT_INT;
                return;
            case tLT:           //"<"
                addInsn(BC_ICMP, left);
                addInsn(BC_ILOAD0);

                bc->addBranch(BC_IFICMPG, yes);
                addInsn(BC_ILOAD0);
                bc->addBranch(BC_JA, end);
                bc->bind(yes);
                addInsn(BC_ILOAD1);
                bc->bind(end);

                tos_type = VT_INT;
                return;
            case tLE:           //"<="
                addInsn(BC_ICMP, left);
                addInsn(BC_ILOAD0);

                bc->addBranch(BC_IFICMPGE, yes);
                addInsn(BC_ILOAD0);
                bc->addBranch(BC_JA, end);
                bc->bind(yes);
                addInsn(BC_ILOAD1);
                bc->bind(end);

                tos_type = VT_INT;
                return;
            case tADD:      //"+"
                addInsn(BC_IADD, left);

                tos_type = left;
                return;
            case tSUB:      //"-"
                addInsn(BC_ISUB, left);

                tos_type = left;
                return;
            case tMUL:      //"*"
                addInsn(BC_IMUL, left);

                tos_type = left;
                return;
            case tDIV:      //"/"
                addInsn(BC_IDIV, left);

                tos_type = left;
                return;
            case tMOD:      //"%"
                assertInt(left);
                addInsn(BC_IMOD);

                tos_type = left;
                return;
            default:
                throw 0;
        }

        throw 0;
    }

    virtual void visitUnaryOpNode(UnaryOpNode * node) {
        node->operand()->visit(this);

        switch (node->kind()) {
            case tNOT: // !
                assertInt(tos_type);
                addInsn(BC_ILOAD1);
                addInsn(BC_IAAND);
                addInsn(BC_ILOAD1);
                addInsn(BC_IAXOR);
                return;
            case tSUB: // -
                assertArithmetic(tos_type);
                addInsn(BC_INEG, tos_type);
                return;
            default:
                throw 0;
        }

        throw 0;
    }

    virtual void visitStringLiteralNode(StringLiteralNode * node) {
        addInsn(BC_SLOAD);
        bc->addUInt16(code->makeStringConstant(node->literal()));
        tos_type = VT_STRING;
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode * node) {
        bc->addDouble(node->literal());
        tos_type = VT_DOUBLE;
    }

    virtual void visitIntLiteralNode(IntLiteralNode * node) {
        bc->addInt64(node->literal());
        tos_type = VT_INT;
    }

    virtual void visitLoadNode(LoadNode * node) { // TODO

    }

    virtual void visitStoreNode(StoreNode * node) { // TODO

    }

    virtual void visitForNode(ForNode * node) { // TODO

    }

    virtual void visitWhileNode(WhileNode * node) {
        Label begin(bc);
        Label end(bc);

        bc->bind(begin);
        node->whileExpr()->visit(this);
        assertInt(tos_type);
        addInsn(BC_ILOAD0);
        bc->addBranch(BC_IFICMPE, end);

        node->loopBlock()->visit(this);
        bc->addBranch(BC_JA, begin);

        bc->bind(end);
    }

    virtual void visitIfNode(IfNode * node) {
        Label els(bc);
        Label end(bc);

        if (!node->elseBlock()) {
            node->ifExpr()->visit(this);
            assertInt(tos_type);
            addInsn(BC_ILOAD0);
            bc->addBranch(BC_IFICMPE, end);
            node->thenBlock()->visit(this);
            bc->bind(end);
        } else {
            node->ifExpr()->visit(this);
            assertInt(tos_type);
            addInsn(BC_ILOAD0);
            bc->addBranch(BC_IFICMPE, els);
            node->thenBlock()->visit(this);
            bc->addBranch(BC_JA, end);
            bc->bind(els);
            node->elseBlock()->visit(this);
            bc->bind(end);
        }
    }

    virtual void visitBlockNode(BlockNode * node) { // TODO

    }

    virtual void visitFunctionNode(FunctionNode * node) { // TODO

    }

    virtual void visitReturnNode(ReturnNode * node) {
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
        }
        addInsn(BC_RETURN);
    }

    virtual void visitCallNode(CallNode * node) { // TODO

    }

    virtual void visitNativeCallNode(NativeCallNode * node) { // TODO

    }

    virtual void visitPrintNode(PrintNode * node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            addInsn(BC_IPRINT, tos_type);
        }
        tos_type = VT_INVALID;
    }

private: // --------------------------------------------- //

    void addInsn(Instruction insn) {
        bc->addInsn(insn);
    }

    void addInsn(Instruction insn, VarType type) {
        bc->addInsn(InsnToType(insn, type));
    }


    bool isIntType(VarType type) {
        return type == VT_INT;
    }

    bool isArithmeticType(VarType type) {
        return type == VT_INT || type == VT_DOUBLE;
    }

    void assertInt(VarType type) {
        if (type != VT_INT) {
            throw 0;
        }
    }

    void assertArithmetic(VarType type) {
        if (type != VT_INT && type != VT_DOUBLE) {
            throw 0;
        }
    }

    void assertSame(VarType left, VarType right) {
        if (left != right) {
            throw 0;
        }
    }

    Instruction InsnToType(Instruction insn, VarType type) {
        switch(insn) {
            #define CASE_IDS(INT, DOUBLE, STRING)                   \
                case BC_##INT: case BC_##DOUBLE: case BC_##STRING:  \
                    switch (type) {                                 \
                        case VT_INT: return BC_##INT;               \
                        case VT_DOUBLE: return BC_##DOUBLE;         \
                        case VT_STRING: return BC_##STRING;         \
                        default: throw 0;                           \
                    }                                               //

            #define CASE_ID(INT, DOUBLE)                            \
                case BC_##INT: case BC_##DOUBLE:                    \
                    switch (type) {                                 \
                        case VT_INT: return BC_##INT;               \
                        case VT_DOUBLE: return BC_##DOUBLE;         \
                        default: throw 0;                           \
                    }                                               //

            CASE_IDS(ILOAD, DLOAD, SLOAD)
            CASE_IDS(ILOAD0, DLOAD0, SLOAD0)
            CASE_ID(ILOAD1, DLOAD1)
            CASE_ID(ILOADM1, DLOADM1)
            CASE_ID(IADD, DADD)
            CASE_ID(ISUB, DSUB)
            CASE_ID(IMUL, DMUL)
            CASE_ID(IDIV, DDIV)
            CASE_ID(INEG, DNEG)
            CASE_IDS(IPRINT, DPRINT, SPRINT)
            CASE_IDS(LOADIVAR0, LOADDVAR0, LOADSVAR0)
            CASE_IDS(LOADIVAR1, LOADDVAR1, LOADSVAR1)
            CASE_IDS(LOADIVAR2, LOADDVAR2, LOADSVAR2)
            CASE_IDS(LOADIVAR3, LOADDVAR3, LOADSVAR3)
            CASE_IDS(STOREIVAR0, STOREDVAR0, STORESVAR0)
            CASE_IDS(STOREIVAR1, STOREDVAR1, STORESVAR1)
            CASE_IDS(STOREIVAR2, STOREDVAR2, STORESVAR2)
            CASE_IDS(STOREIVAR3, STOREDVAR3, STORESVAR3)
            CASE_IDS(LOADIVAR, LOADDVAR, LOADSVAR)
            CASE_IDS(STOREIVAR, STOREDVAR, STORESVAR)
            CASE_IDS(LOADCTXIVAR, LOADCTXDVAR, LOADCTXSVAR)
            CASE_IDS(STORECTXIVAR, STORECTXDVAR, STORECTXSVAR)
            CASE_ID(ICMP, DCMP)

            #undef CASE_IDS
            #undef CASE_ID

            default:
                throw 0;
        }

        throw 0;
    }

};

}