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

    void Add(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch" << std::endl;
        }

        Instruction instr = BC_INVALID;
        switch (leftType) {
            case VT_DOUBLE:
                instr = BC_DADD;
                break;
            case VT_INT:
                instr = BC_IADD;
                break;
            case VT_VOID:
            case VT_STRING:
                instr = BC_INVALID;
                break;
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
        }
        out->addInsn(instr);
    }

    void Sub(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch" << std::endl;
        }

        Instruction instr = BC_INVALID;
        switch (leftType) {
            case VT_DOUBLE:
                instr = BC_DSUB;
                break;
            case VT_INT:
                instr = BC_ISUB;
                break;
            case VT_VOID:
            case VT_STRING:
                instr = BC_INVALID;
                break;
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
        }
        out->addInsn(instr);
    }

    void Mul(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch" << std::endl;
        }

        Instruction instr = BC_INVALID;
        switch (leftType) {
            case VT_DOUBLE:
                instr = BC_DMUL;
                break;
            case VT_INT:
                instr = BC_IMUL;
                break;
            case VT_VOID:
            case VT_STRING:
                instr = BC_INVALID;
                break;
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
        }
        out->addInsn(instr);
    }

    void Div(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch" << std::endl;
        }

        Instruction instr = BC_INVALID;
        switch (leftType) {
            case VT_DOUBLE:
                instr = BC_DDIV;
                break;
            case VT_INT:
                instr = BC_IDIV;
                break;
            case VT_VOID:
            case VT_STRING:
                instr = BC_INVALID;
                break;
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
        }
        out->addInsn(instr);
    }

    void Mod(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch" << std::endl;
        }

        Instruction instr = BC_INVALID;
        switch (leftType) {
            case VT_INT:
                instr = BC_IMOD;
                break;
            case VT_VOID:
            case VT_DOUBLE:
            case VT_STRING:
                instr = BC_INVALID;
                break;
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << leftType
                          << "'"
                          << std::endl;
                break;
        }
        out->addInsn(instr);
    }

    void CmpEq(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        Cmp(out, BC_IFICMPE, leftType, rightType);
    }

    void CmpNeq(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        Cmp(out, BC_IFICMPNE, leftType, rightType);
    }

    void CmpGt(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        Cmp(out, BC_IFICMPG, leftType, rightType);
    }

    void CmpGe(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        Cmp(out, BC_IFICMPGE, leftType, rightType);
    }

    void CmpLt(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        Cmp(out, BC_IFICMPL, leftType, rightType);
    }

    void CmpLe(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        Cmp(out, BC_IFICMPLE, leftType, rightType);
    }

    void Not(Bytecode* out, VarType type = VT_VOID) {
        out->addInsn(BC_ILOAD0);
        CmpEq(out);
    }

private:

    void Cmp(Bytecode* out, Instruction cmpInsn, VarType leftType, VarType rightType) {
        out->addInsn(cmpInsn);
        const uint16_t jump_offset_size = sizeof(uint16_t);
        out->addInt16(jump_offset_size + InsnSize[BC_ILOAD0] + InsnSize[BC_JA]);
        // :
        out->addInsn(BC_ILOAD0);    // push "0"
        out->addInsn(BC_JA);
        out->addUInt16(jump_offset_size + InsnSize[BC_ILOAD1]);
        // :
        out->addInsn(BC_ILOAD1);    // push "1"

//                out->addInsn(BC_SWAP);
//                out->addInsn(BC_POP);
//                out->addInsn(BC_SWAP);
//                out->addInsn(BC_POP);
    }

    static const uint8_t InsnSize[];
};

} /* namespace mathvm_ext */
#endif /* BYTECODEINSTRUCTIONPRIMITIVES_H_ */
