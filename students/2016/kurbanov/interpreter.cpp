#include "include/interpreter.h"

void Interpreter::executeInstruction() {
    Instruction instruction = currentBytecode()->getInsn(programCounter());
    next();

    switch (instruction) {
        case BC_INVALID: throw ExecutionException("Invalid instruction"); break;
        case BC_DLOAD: dload(); break;
        case BC_ILOAD: iload(); break;
        case BC_SLOAD: sload(); break;
        case BC_DLOAD0: dload0(); break;
        case BC_ILOAD0: iload0(); break;
        case BC_SLOAD0: sload0(); break;
        case BC_ILOAD1: iload1(); break;
        case BC_DLOAD1: dload1(); break;
        case BC_ILOADM1: iloadm1(); break;
        case BC_DLOADM1: dloadm1(); break;
        case BC_IADD: iadd(); break;
        case BC_DADD: dadd(); break;
        case BC_ISUB: isub(); break;
        case BC_DSUB: dsub(); break;
        case BC_IMUL: imul(); break;
        case BC_DMUL: dmul(); break;
        case BC_IDIV: idiv(); break;
        case BC_DDIV: ddiv(); break;
        case BC_IMOD: imod(); break;
        case BC_INEG: ineg(); break;
        case BC_DNEG: dneg(); break;
        case BC_IAOR: iaor(); break;
        case BC_IAAND: iaand(); break;
        case BC_IAXOR: iaxor(); break;
        case BC_IPRINT: iprint(); break;
        case BC_DPRINT: dprint(); break;
        case BC_SPRINT: sprint(); break;
        case BC_I2D: i2d(); break;
        case BC_D2I: d2i(); break;
        case BC_SWAP: swap(); break;
        case BC_POP: pop(); break;
        case BC_LOADDVAR: loaddvar(); break;
        case BC_LOADIVAR: loadivar(); break;
        case BC_LOADSVAR: loadsvar(); break;
        case BC_STOREDVAR: storedvar(); break;
        case BC_STOREIVAR: storeivar(); break;
        case BC_STORESVAR: storesvar(); break;
        case BC_LOADCTXDVAR: loadctxdvar(); break;
        case BC_LOADCTXIVAR: loadctxivar(); break;
        case BC_LOADCTXSVAR: loadctxsvar(); break;
        case BC_STORECTXDVAR: storectxdvar(); break;
        case BC_STORECTXIVAR: storectxivar(); break;
        case BC_STORECTXSVAR: storectxsvar(); break;
        case BC_DCMP: dcmp(); break;
        case BC_ICMP: icmp(); break;
        case BC_JA: ja(); break;
        case BC_IFICMPNE:  ificmpne(); break;
        case BC_IFICMPE: ificmpe(); break;
        case BC_IFICMPG: ificmpg(); break;
        case BC_IFICMPGE: ificmpge(); break;
        case BC_IFICMPL: ificmpl(); break;
        case BC_IFICMPLE: ificmple(); break;
        case BC_DUMP: dump(); break;
        case BC_STOP: throw ExecutionException("Execution stopped");
        case BC_CALL: call(); break;
        case BC_CALLNATIVE: throw ExecutionException("Natives are not supported");
        case BC_RETURN: bc_return(); break;
        case BC_BREAK: next(); break;
        default: break;
    }
}