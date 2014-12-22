#include "interpreter_code_impl.h"
#include "ast.h"
#include <vector>
#include <tic.h>


namespace mathvm {
    struct AstVarInfo {
        uint16_t contextId;
        uint16_t id;

        AstVarInfo(uint16_t contextId,
                uint16_t id) : contextId(contextId), id(id) {
        }
    };

    struct AstFunctionInfo {
        BytecodeFunction *function;

        AstFunctionInfo(BytecodeFunction *function) :
                function(function) {
        }
    };


    class ByteCodeVisitor : public AstVisitor, public ErrorInfoHolder {
#define INSN(instruction) (topStack() == VT_INT ? insn(BC_I##instruction) : insn(BC_D##instruction))

        //fields///////////////////////////////////////
        InterpreterCodeImpl *interpreterCode;
        Status *currStatus;
        BytecodeFunction *currentBytecodeFunction = 0;
        Scope *currScope = 0;
        std::vector<VarType> *currStack;

//utils///////////////////////////////////////////////
        void error(const char *format, ...) {
            va_list args;
            va_start(args, format);
            verror(currentBytecodeFunction->id(), format, args);
        }

        void pushStack(VarType type) {
            currStack->push_back(type);
        }

        VarType popStack() {
            VarType top = topStack();
            currStack->pop_back();
            return top;
        }

        VarType topStack() {
            return currStack->back();
        }

        void replaceTop(VarType v) {
            popStack();
            pushStack(v);
        }


        void insn(Instruction instr) {
            if (!bytecode()) {
                error("null byte -> code can't add instruction");
            }
            bytecode()->addInsn(instr);
        }

        void branch(Instruction insn, Label &target) {
            bytecode()->addBranch(insn, target);
        }

        template<class T>
        void typed(T d) {
            if (!bytecode()) {
                error("null byte code can't add instruction");
            }
            bytecode()->addTyped(d);
        }


        void store(const AstVar *var) {
            AstVarInfo *varInfo = (AstVarInfo *) var->info();
            cast(topStack(), var->type());
            if (varInfo->contextId == currentBytecodeFunction->id()) {
                switch (var->type()) {
                    case VT_INT:
                        insn(BC_STOREIVAR);
                        break;
                    case VT_DOUBLE:
                        insn(BC_STOREDVAR);
                        break;
                    case VT_STRING:
                        insn(BC_STORESVAR);
                        break;
                }
            } else {
                switch (var->type()) {
                    case VT_INT:
                        insn(BC_STORECTXIVAR);
                        break;
                    case VT_DOUBLE:
                        insn(BC_STORECTXDVAR);
                        break;
                    case VT_STRING:
                        insn(BC_STORECTXSVAR);
                        break;
                }
                typed(varInfo->contextId);
            }
            typed(varInfo->id);
            popStack();
        }

        void store0(VarType v) {
            switch (v) {
                case VT_INT:
                    insn(BC_STOREIVAR0);
                    break;
                case VT_DOUBLE:
                    insn(BC_STOREDVAR0);
                    break;
                case VT_STRING:
                    insn(BC_STORESVAR0);
                    break;
            }
            popStack();
        }

        void store(const AstVar *var, TokenKind kind) {
            if (kind != tASSIGN) {}
            switch (kind) {
                case tINCRSET: {
                    load(var);
                    INSN(ADD);
                    break;
                }
                case tDECRSET: {
                    load(var);
                    INSN(SUB);
                    break;
                };
            }
            store(var);
        }

        void load(const AstVar *var) {
            AstVarInfo *varInfo = (AstVarInfo *) var->info();
            if (varInfo->contextId == currentBytecodeFunction->id()) {
                switch (var->type()) {
                    case VT_INT:
                        insn(BC_LOADIVAR);
                        break;
                    case VT_DOUBLE:
                        insn(BC_LOADDVAR);
                        break;
                    case VT_STRING:
                        insn(BC_LOADSVAR);
                        break;
                }
            } else {
                switch (var->type()) {
                    case VT_INT:
                        insn(BC_LOADCTXIVAR);
                        break;
                    case VT_DOUBLE:
                        insn(BC_LOADCTXDVAR);
                        break;
                    case VT_STRING:
                        insn(BC_LOADCTXSVAR);
                        break;
                }
                typed(varInfo->contextId);
            }
            typed(varInfo->id);
            pushStack(var->type());
        }

        void load0(VarType v) {
            switch (v) {
                case VT_INT:
                    insn(BC_LOADIVAR0);
                    break;
                case VT_DOUBLE:
                    insn(BC_LOADDVAR0);
                    break;
                case VT_STRING:
                    insn(BC_LOADSVAR0);
                    break;
            }
            pushStack(v);
        }

        void cast(VarType source, VarType wanted) {
            if (source == wanted) return;
            switch (source) {
                case VT_INT:
                    if (wanted == VT_DOUBLE) {
                        insn(BC_I2D);
                        replaceTop(wanted);
                    }
                    return;
                case VT_DOUBLE:
                    if (wanted == VT_INT) {
                        insn(BC_D2I);
                        replaceTop(wanted);
                    }
                    return;
            }
            error("can't cast %s to %s", typeToName(source), typeToName(wanted));
        }

        void booleanize() {
            if (topStack() != VT_INT) {
                error("cant booleanize not int");
            }
            insn(BC_ILOAD0);
            insn(BC_ICMP);
            insn(BC_ILOAD1);
            insn(BC_IAAND);
        }

        void booleanizePair() {
            booleanize();
            insn(BC_SWAP);
            booleanize();
        }

        VarType uniformTopPair() {
            VarType upper = topStack();
            VarType lower = (*currStack)[currStack->size() - 2];
            if (lower == upper && (lower == VT_INT || lower == VT_DOUBLE)) {
                return lower;
            }
            if (lower != VT_DOUBLE && upper != VT_DOUBLE) {
                error("cast exception: cant uniform cast top pair %s %s", typeToName(lower), typeToName(lower));
            }
            if (lower != VT_DOUBLE) {
                insn(BC_SWAP);
                cast(lower, VT_DOUBLE);
                insn(BC_SWAP);
            } else {
                cast(upper, VT_DOUBLE);
            }
            return VT_DOUBLE;
        }

        bool isLogic(TokenKind kind) {
            return kind == tOR || kind == tAND;
        }

        bool isIntOp(TokenKind kind) {
            if (kind == tMOD || isLogic(kind)) {
                return true;
            }
            return (kind >= tAOR && kind <= tAXOR);
        }

        bool isCompareOp(TokenKind kind) {
            return kind >= tEQ && kind <= tLE;
        }

        void binaryOp(TokenKind kind) {
            VarType type;
            if (isIntOp(kind)) {
                VarType right = (*currStack)[currStack->size() - 2];
                VarType left = topStack();
                if (right != VT_INT || left != VT_INT) {
                    error("cant do int operation on not int args  %s", tokenOp(kind));
                }
                type = VT_INT;
                if (isLogic(kind)) {
                    booleanizePair();
                }
                switch (kind) {
                    case tMOD:
                        insn(BC_IMOD);
                        break;
                    case tOR:
                        insn(BC_IADD);
                        booleanize();
                        break;
                    case tAND:
                        insn(BC_IMUL);
                        break;
                    case tAAND:
                        insn(BC_IAAND);
                        break;
                    case tAOR:
                        insn(BC_IAOR);
                        break;
                    case tAXOR:
                        insn(BC_IAXOR);
                        break;
                    default:
                        error("failed on translate bynary int op %s", tokenOp(kind));
                }
            }
            else {
                type = uniformTopPair();//here we cast to upper type;
                if (isCompareOp(kind)) {
                    INSN(CMP);//become -1 , 0 ,1 on top
                    type = VT_INT;
                    if (kind == tNEQ) {
                        booleanize(); // simply -1 -> 1
                    } else {
                        Label trueLabel(bytecode());
                        Label endLabel(bytecode());
                        insn(BC_ILOAD0);
//                        insn(BC_SWAP);//cmp(0,cmp(left,right)) -> 0 , 1
                        switch (kind) {
                            case tLT://<
                                branch(BC_IFICMPG, trueLabel);
                                break;
                            case tLE://<=
                                branch(BC_IFICMPGE, trueLabel);
                                break;
                            case tEQ://==
                                branch(BC_IFICMPE, trueLabel);
                                break;
                            case tGT://>
                                branch(BC_IFICMPL, trueLabel);
                                break;
                            case tGE://>=
                                branch(BC_IFICMPLE, trueLabel);
                                break;

                        }
                        insn(BC_ILOAD0);// reachable on fail
                        branch(BC_JA, endLabel);
                        bytecode()->bind(trueLabel);
                        insn(BC_ILOAD1);
                        bytecode()->bind(endLabel);
                    }
                } else {
                    switch (kind) {
                        case tADD:
                            INSN(ADD);
                            break;
                        case tSUB:
                            INSN(SUB);
                            break;
                        case tMUL:
                            INSN(MUL);
                            break;
                        case tDIV:
                            INSN(DIV);
                            break;
                        default:
                            error("unkonwn binary opertaion %s", tokenOp(kind));
                    }
                }

            }
            popStack();
            popStack();
            pushStack(type);
        }

        void unaryOp(TokenKind kind) {
            if (kind == tNOT) {
                if (topStack() != VT_INT) {
                    error("! to not int");
                }
                Label l_true(bytecode());
                Label l_end(bytecode());
                insn(BC_ILOAD0);
                branch(BC_IFICMPE, l_true);
                insn(BC_ILOAD0);
                branch(BC_JA, l_end);
                bytecode()->bind(l_true);
                insn(BC_ILOAD1);
                bytecode()->bind(l_end);
                return;
            }
            if (kind == tSUB) {
                INSN(NEG);
                return;
            }
            error("unkown unary operation %s", tokenOp(kind));
        }

        void printTop() {
            if (currStack->empty()) {
                error("don't know what to print");
            }
            switch (topStack()) {
                case VT_INT:
                    insn(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    insn(BC_DPRINT);
                    break;
                case VT_STRING:
                    insn(BC_SPRINT);
                    break;
                default:
                    error("don't know how to print %s", typeToName(topStack()));
            }
            popStack();
        }

        //param initers
        void initFunctionsAndParameters(AstFunction *function);

        void initVars(Scope *scope);

        void initChildFunctions(Scope *scope);

        void initLocals(BlockNode *blockNode);

        //visitors declaration

        void visitForNode(mathvm::ForNode *node);

        void visitPrintNode(mathvm::PrintNode *node);

        void visitLoadNode(mathvm::LoadNode *node);

        void visitIfNode(mathvm::IfNode *node);

        void visitIntLiteralNode(mathvm::IntLiteralNode *node);

        void visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node);

        void visitStringLiteralNode(mathvm::StringLiteralNode *node);

        void visitWhileNode(mathvm::WhileNode *node);

        void visitBlockNode(mathvm::BlockNode *node);

        void visitBinaryOpNode(mathvm::BinaryOpNode *node);

        void visitUnaryOpNode(mathvm::UnaryOpNode *node);

        void visitNativeCallNode(mathvm::NativeCallNode *node);

        void visitFunctionNode(mathvm::FunctionNode *node);

        void visitReturnNode(mathvm::ReturnNode *node);

        void visitStoreNode(mathvm::StoreNode *node);

        void visitCallNode(mathvm::CallNode *node);

        Bytecode *bytecode() {
            return currentBytecodeFunction->bytecode();
        }

    public:

        Status *process(AstFunction *main, InterpreterCodeImpl **code) {
            currStatus = Status::Ok();
            interpreterCode = *code;
            initFunctionsAndParameters(main);
            visitFunctionNode(main->node());
            return currStatus;
        }
    };


}