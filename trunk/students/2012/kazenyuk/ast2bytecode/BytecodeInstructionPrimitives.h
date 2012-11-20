#ifndef BYTECODEINSTRUCTIONPRIMITIVES_H_
#define BYTECODEINSTRUCTIONPRIMITIVES_H_

#include "mathvm.h"
#include "ast.h"

namespace mathvm_ext {

using namespace mathvm;

class BytecodeInstructionPrimitives {
public:
    BytecodeInstructionPrimitives();
    virtual ~BytecodeInstructionPrimitives();

    void CmpEq(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
//        switch (node->var()->type()) {
//                    case VT_INVALID:
//                        instr = BC_INVALID;
//                        break;
//                    case VT_VOID:
//                        instr = BC_INVALID;
//                        break;
//                    case VT_DOUBLE:
//                        instr = BC_LOADDVAR;
//                        break;
//                    case VT_INT:
//                        instr = BC_LOADIVAR;
//                        break;
//                    case VT_STRING:
//                        instr = BC_LOADSVAR;
//                        break;
//                    default:
//                        instr = BC_INVALID;
//                        std::cerr << "Error: Unknown AST var type '"
//                                  << node->var()->type()
//                                  << "'"
//                                  << std::endl;

//                out->addBranch(BC_IFICMPG, *(new Label()));
        out->addInsn(BC_IFICMPE);
        Cmp(out, leftType, rightType);
    }

    void CmpNeq(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        out->addInsn(BC_IFICMPNE);
        Cmp(out, leftType, rightType);
    }

    void CmpGt(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        out->addInsn(BC_IFICMPG);
        Cmp(out, leftType, rightType);
    }

    void CmpGe(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        out->addInsn(BC_IFICMPGE);
        Cmp(out, leftType, rightType);
    }

    void CmpLt(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        out->addInsn(BC_IFICMPL);
        Cmp(out, leftType, rightType);
    }

    void CmpLe(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        out->addInsn(BC_IFICMPLE);
        Cmp(out, leftType, rightType);
    }

    void Cmp(Bytecode* out, VarType leftType, VarType rightType) {
        out->addUInt16(InsnSize[BC_ILOAD0] + InsnSize[BC_JA]);
        // :
        out->addInsn(BC_ILOAD0);    // push "0"
        out->addInsn(BC_JA);
        out->addUInt16(InsnSize[BC_ILOAD1]);
        // :
        out->addInsn(BC_ILOAD1);    // push "1"

//                out->addInsn(BC_SWAP);
//                out->addInsn(BC_POP);
//                out->addInsn(BC_SWAP);
//                out->addInsn(BC_POP);
    }

    void Not(Bytecode* out, VarType type = VT_VOID) {
        out->addInsn(BC_ILOAD0);

        CmpEq(out);
    }

    static const uint8_t InsnSize[];
};

} /* namespace mathvm_ext */
#endif /* BYTECODEINSTRUCTIONPRIMITIVES_H_ */
