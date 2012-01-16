#ifndef MYCODE_H
#define MYCODE_H

class MyCode: public mathvm::Code {
public:
    virtual mathvm::Status* execute(std::vector<mathvm::Var*>& vars) {
        return 0;
    }

    void SetMainBytecode(mathvm::Bytecode const & bytecode) {
        myBytecode = bytecode;
    }

    mathvm::Bytecode const & GetMainBytecode() const {
        return myBytecode;
    }

private:
    mathvm::Bytecode myBytecode;
};

#endif // MYCODE_H
