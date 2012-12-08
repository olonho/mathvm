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

    VarType Add(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch: left is "
                      << typeToName(leftType)
                      << " but right is"
                      << typeToName(rightType)
                      << std::endl;
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

        return leftType;
    }

    VarType Sub(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch: left is "
                      << typeToName(leftType)
                      << " but right is"
                      << typeToName(rightType)
                      << std::endl;
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

        return leftType;
    }

    VarType Mul(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch: left is "
                      << typeToName(leftType)
                      << " but right is"
                      << typeToName(rightType)
                      << std::endl;
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

        return leftType;
    }

    VarType Div(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch: left is "
                      << typeToName(leftType)
                      << " but right is"
                      << typeToName(rightType)
                      << std::endl;
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

        return leftType;
    }

    VarType Mod(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        if (leftType != rightType) {
            std::cerr << "Error: Type mismatch: left is "
                      << typeToName(leftType)
                      << " but right is"
                      << typeToName(rightType)
                      << std::endl;
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

        return leftType;
    }

    VarType Neg(Bytecode* out, VarType type = VT_VOID) {
        Instruction instr = BC_INVALID;
        switch (type) {
            case VT_DOUBLE:
                instr = BC_DNEG;
                break;
            case VT_INT:
                instr = BC_INEG;
                break;
            case VT_VOID:
            case VT_STRING:
                instr = BC_INVALID;
                std::cerr << "Error: Unsupported type of the argument"
                        << std::endl;
                break;
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid AST var type '"
                          << type
                          << "'"
                          << std::endl;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << type
                          << "'"
                          << std::endl;
                break;
        }
        out->addInsn(instr);

        return type;
    }

    VarType CmpEq(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        return Cmp(out, BC_IFICMPE, leftType, rightType);
    }

    VarType CmpNeq(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        return Cmp(out, BC_IFICMPNE, leftType, rightType);
    }

    VarType CmpGt(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        return Cmp(out, BC_IFICMPG, leftType, rightType);
    }

    VarType CmpGe(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        return Cmp(out, BC_IFICMPGE, leftType, rightType);
    }

    VarType CmpLt(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        return Cmp(out, BC_IFICMPL, leftType, rightType);
    }

    VarType CmpLe(Bytecode* out, VarType leftType = VT_VOID, VarType rightType = VT_VOID) {
        return Cmp(out, BC_IFICMPLE, leftType, rightType);
    }

    VarType Not(Bytecode* out, VarType type = VT_VOID) {
        out->addInsn(BC_ILOAD0);
        return CmpEq(out);
    }

    void Store(Bytecode* out, uint16_t varId, VarType varType) {
        Instruction instr = BC_INVALID;

        // get value from the top of the stack and store in the VAR
        switch (varType) {
            case VT_INVALID:
                instr = BC_INVALID;
                break;
            case VT_VOID:
                instr = BC_INVALID;
                break;
            case VT_DOUBLE:
                instr = BC_STOREDVAR;
                break;
            case VT_INT:
                instr = BC_STOREIVAR;
                break;
            case VT_STRING:
                instr = BC_STORESVAR;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown type '"
                          << varType
                          << "'"
                          << std::endl;
                break;
        }

        out->addInsn(instr);
        if (instr != BC_INVALID) {
            out->addUInt16(varId);
        }
    }

    VarType Load(Bytecode* out, uint16_t varId, VarType varType) {
        Instruction instr = BC_INVALID;

        switch (varType) {
            case VT_INVALID:
                instr = BC_INVALID;
                break;
            case VT_VOID:
                instr = BC_INVALID;
                break;
            case VT_DOUBLE:
                instr = BC_LOADDVAR;
                break;
            case VT_INT:
                instr = BC_LOADIVAR;
                break;
            case VT_STRING:
                instr = BC_LOADSVAR;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown type '"
                          << varType
                          << "'"
                          << std::endl;
                break;
        }

        out->addInsn(instr);
        if (instr != BC_INVALID) {
            out->addUInt16(varId);
        }

        return varType;
    }

    void Inc(Bytecode* out, uint16_t varId, VarType varType) {
        // NOTE: inc argument is already on the top of the stack

        // load
        Load(out, varId, varType);
        // inc
        Add(out, varType, varType);
        // store back
        Store(out, varId, varType);
    }

    void Dec(Bytecode* out, uint16_t varId, VarType varType) {
        // load
        Load(out, varId, varType);
        // inc
        Sub(out, varType, varType);
        // store back
        Store(out, varId, varType);
    }

    void Print(Bytecode* out, VarType type = VT_VOID) {
        Instruction instr = BC_INVALID;

        switch (type) {
            case VT_INVALID:
                instr = BC_INVALID;
                std::cerr << "Error: Invalid var type '"
                          << type
                          << "'"
                          << std::endl;
                break;
            case VT_VOID:
                instr = BC_INVALID;
                std::cerr << "Error: Unsupported type of the argument"
                        << std::endl;
                break;
            case VT_DOUBLE:
                instr = BC_DPRINT;
                break;
            case VT_INT:
                instr = BC_IPRINT;
                break;
            case VT_STRING:
                instr = BC_SPRINT;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << type
                          << "'"
                          << std::endl;
                break;
        }

        out->addInsn(instr);
    }

    VarType Invalid(Bytecode* out) {
        std::cerr << "Warning: emitting BC_INVALID" << std::endl;
        out->addInsn(BC_INVALID);

        return VT_INVALID;
    }

private:

    VarType Cmp(Bytecode* out, Instruction cmpInsn, VarType leftType, VarType rightType) {
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
        return VT_INT;
    }

    static const uint8_t InsnSize[];
};

} /* namespace mathvm_ext */
#endif /* BYTECODEINSTRUCTIONPRIMITIVES_H_ */
