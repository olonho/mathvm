//
// Created by svloyso on 17.10.16.
//

#include <fstream>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <sstream>

#include "ast_to_code.h"
#include "parser.h"

int getdir (std::string dir, std::vector<std::string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        std::cout << "Error(" << errno << ") opening " << dir << std::endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if(std::string(dirp->d_name) != "." && std::string(dirp->d_name) != "..")
            files.push_back(std::string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

namespace mathvm {
class AstComparer : public AstVisitor {
    AstNode* comparable;
public:

    Status* compare(AstNode* left, AstNode* right) {
        comparable = right;
        try {

        } catch (Status* status) {
            return status;
        }
        return Status::Ok();
    }

    void throwError(AstNode* node) {
        throw Status::Error("Different Ast nodes", node->position());
    }

    void visitUnaryOpNode(UnaryOpNode *node) {
        if(!comparable->isUnaryOpNode()) throwError(node);
        UnaryOpNode* compNode = comparable->asUnaryOpNode();
        comparable = compNode->operand();
        node->operand()->visit(this);
    }

    void visitBinaryOpNode(BinaryOpNode *node) {
        if(!comparable->isBinaryOpNode()) throwError(node);
        BinaryOpNode* compNode = comparable->asBinaryOpNode();
        comparable = compNode->left();
        node->left()->visit(this);
        comparable = compNode->right();
        node->right()->visit(this);
    }

    void visitStringLiteralNode(StringLiteralNode *node) {
        if(!comparable->isStringLiteralNode()) throwError(node);
        StringLiteralNode* compNode = comparable->asStringLiteralNode();
        if(node->literal() != compNode->literal()) throwError(node);
    }

    void visitIntLiteralNode(IntLiteralNode *node) {
        if(!comparable->isIntLiteralNode()) throwError(node);
        IntLiteralNode* compNode = comparable->asIntLiteralNode();
        if(node->literal() != compNode->literal()) throwError(node);
    }

    void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        if(!comparable->isDoubleLiteralNode()) throwError(node);
        DoubleLiteralNode* compNode = comparable->asDoubleLiteralNode();
        if(node->literal() != compNode->literal()) throwError(node);
    }

    void visitLoadNode(LoadNode *node) {
        if(!comparable->isLoadNode()) throwError(node);
        LoadNode* compNode = comparable->asLoadNode();
        if(node->var()->name() != compNode->var()->name()) throwError(node);
    }

    void visitStoreNode(StoreNode *node) {
        if(!comparable->isStoreNode()) throwError(node);
        StoreNode* compNode = comparable->asStoreNode();
        if(node->var()->name() != compNode->var()->name()) throwError(node);
        comparable = compNode->value();
        node->value()->visit(this);
    }

    void visitBlockNode(BlockNode *node) {
        if(!comparable->isBlockNode()) throwError(node);
        BlockNode* compNode = comparable->asBlockNode();

        if(node->scope()->variablesCount() != compNode->scope()->variablesCount()
            || node->scope()->functionsCount() != node->scope()->functionsCount()
            || node->nodes() != compNode->nodes()) throwError(node);

        for(Scope::VarIterator it1(node->scope()), it2(compNode->scope()); it1.hasNext();) {
            AstVar* v1 = it1.next();
            AstVar* v2 = it2.next();
            if(v1->name() != v2->name() || v1->type() != v2->type()) throwError(node);
        }

        for(Scope::FunctionIterator it1(node->scope()), it2(compNode->scope()); it1.hasNext();) {
            AstFunction* f1 = it1.next();
            AstFunction* f2 = it2.next();
            comparable = f2->node();
            f1->node()->visit(this);
        }

        for(int i = 0; i < node->nodes(); ++i) {
            comparable = compNode->nodeAt(i);
            node->nodeAt(i)->visit(this);
        }
    }

    void visitNativeCallNode(NativeCallNode *node) {
        if(!comparable->isNativeCallNode()) throwError(node);
        NativeCallNode* cmpNode = comparable->asNativeCallNode();

        if(node->nativeName() != cmpNode->nativeName()) throwError(node);
    }

    void visitForNode(ForNode *node) {
        if(!comparable->isForNode()) throwError(node);
        ForNode* cmpNode = comparable->asForNode();
        if(cmpNode->var()->name() != node->var()->name()) throwError(node);
        comparable = cmpNode->inExpr();
        node->inExpr()->visit(this);
        comparable = cmpNode->body();
        node->body()->visit(this);
    }

    void visitWhileNode(WhileNode *node) {
        if(!comparable->isWhileNode()) throwError(node);
        WhileNode* cmpNode = comparable->asWhileNode();
        comparable = cmpNode->whileExpr();
        node->whileExpr()->visit(this);
        comparable = cmpNode->loopBlock();
        node->loopBlock()->visit(this);
    }

    void visitIfNode(IfNode *node) {
        if(!comparable->isIfNode()) throwError(node);
        IfNode* cmpNode = comparable->asIfNode();
        comparable = cmpNode->ifExpr();
        node->ifExpr()->visit(this);
        comparable = cmpNode->thenBlock();
        node->thenBlock()->visit(this);
        if((bool)cmpNode->elseBlock() != (bool)node->elseBlock()) throwError(node);
        if(node->elseBlock()) {
            comparable = cmpNode->elseBlock();
            node->elseBlock()->visit(this);
        }
    }

    void visitReturnNode(ReturnNode *node) {
        if(!comparable->isReturnNode()) throwError(node);
        ReturnNode* cmpNode = comparable->asReturnNode();
        comparable = cmpNode->returnExpr();
        node->returnExpr()->visit(this);
    }

    void visitFunctionNode(FunctionNode *node) {
        if(!comparable->isFunctionNode()) throwError(node);
        FunctionNode* cmpNode = comparable->asFunctionNode();
        if(node->name() != cmpNode->name() || node->parametersNumber() != cmpNode->parametersNumber()) throwError(node);
        for(int i = 0; i < node->parametersNumber(); ++i) {
            if(node->parameterName(i) != cmpNode->parameterName(i)
               || node->parameterType(i) != cmpNode->parameterType(i)) throwError(node);
        }
        comparable = cmpNode->body();
        node->body()->visit(this);
    }

    void visitCallNode(CallNode *node) {
        if(!comparable->isCallNode()) throwError(node);
        CallNode* cmpNode = comparable->asCallNode();

        if(node->name() != cmpNode->name() || node->parametersNumber() != cmpNode->parametersNumber()) throwError(node);
        for(int i = 0; i < node->parametersNumber(); ++i) {
            comparable = cmpNode->parameterAt(i);
            node->parameterAt(i)->visit(this);
        }
    }

    void visitPrintNode(PrintNode *node) {
        if(!comparable->isPrintNode()) throwError(node);
        PrintNode* cmpNode = comparable->asPrintNode();

        if(node->operands() != cmpNode->operands()) throwError(node);
        for(int i = 0; i < node->operands(); ++i) {
            comparable = cmpNode->operandAt(i);
            node->operandAt(i)->visit(this);
        }
    }
};
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Give me a test folder as an argument" << std::endl;
        return 1;
    }
    const std::string test_dir = argv[1];

    bool verbose = argc > 2 && argv[2] == std::string("-v");
    
    std::vector<std::string> files;
    getdir(test_dir, files);

    if(files.size() == 0) {
        std::cout << "No tests found. Exit." << std::endl;
    } else {
        std::cout << files.size() << " tests found.";
    }

    int failed = 0;
    
    for(int i = 0; i < files.size(); ++i) {
        std::string filename(files[i]);
        std::cout << "Starting Test #" << i + 1<< ". (" << filename << ")" << std::endl;
        std::ifstream file(test_dir + "/" + filename);
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        if(verbose) {
            std::cout << "Test source:" << std::endl;
            std::cout << content << std::endl;
        }

        mathvm::Parser parser;
        mathvm::Status* parsingStatus = parser.parseProgram(content);
        if (!parsingStatus->isOk()) {
            std::cout << "Test #" << i + 1 << " failed." <<std::endl;
            std::cout << "Parsing failed. Message: " << std::endl;
            std::cout << parsingStatus->getPosition() << " : " << parsingStatus->getError();
            failed += 1;
            continue;
        }
        
        std::stringstream result;
        mathvm::AstToCode astToCode(result);
        astToCode.dumpCode(parser.top()->node()->asFunctionNode()->body());
        mathvm::Parser resParser;
        mathvm::Status* resStatus = resParser.parseProgram(result.str());

        if(verbose) {
            std::cout << "Code got from AST:" << std::endl;
            std::cout << result.str() << std::endl;
        }

        if(!resStatus->isOk()) {
            std::cout << "Test #" << i + 1 << " failed." << std::endl;
            std::cout << "Parsing decompiled code failed. Message: " << std::endl;
            std::cout << resStatus->getPosition() << " : " << resStatus->getError() << std::endl;
            std::string res(result.str());
            res = std::string(res.begin() + resStatus->getPosition() - 10, res.begin() + resStatus->getPosition() + 10);
            std::cout << res << std::endl;
            failed += 1;
            continue;
        }
        mathvm::AstComparer comp;
        mathvm::Status* compStatus = comp.compare(parser.top()->node(), resParser.top()->node());
        if(!compStatus->isOk()) {
            std::cout << "Test #" << i + 1 << " failed." << std::endl;
            std::cout << "Failed AST-tree comparsion." << std::endl;
            std::cout << compStatus->getPosition() << " : " << compStatus->getError() << std::endl;
            failed += 1;
            continue;
        }
    }
    std::cout << "Failed: " << failed << ", Passed: " << (files.size() - failed) << std::endl;
}