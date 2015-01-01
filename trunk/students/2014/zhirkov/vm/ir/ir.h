#pragma once

#include <vector>
#include <memory>
#include <deque>
#include <set>
#include <elf.h>
#include "../util.h"

namespace mathvm {
    namespace IR {

        typedef uint64_t VarId;

#define IR_COMMON_FUNCTIONS(ir) \
virtual IrElement* visit(IrVisitor *const v) const { return v->visit(this); }\
virtual bool is##ir() const { return true; } ;\
virtual ir const* as##ir() const { return this; } ;\
virtual IrType getType() const { return IT_##ir; }

#define FOR_IR(DO) \
        DO(BinOp)\
        DO(UnOp)\
        DO(Variable)\
        DO(Return)\
        DO(Phi)\
        DO(Int)\
        DO(Double)\
        DO(Ptr)\
        DO(Block)\
        DO(Assignment)\
        DO(Call)\
        DO(Print)\
        DO(FunctionRecord)\
        DO(JumpAlways)\
        DO(JumpCond)\
        DO(WriteRef)\
        DO(ReadRef)

#define DECLARE_IR(ir) struct ir;
        FOR_IR(DECLARE_IR)
#undef DECLARE_IR

        struct IrVisitor;
        struct Expression;
        struct Atom;

        class IrElement {
        public:
            enum IrType {

#define DECLARE_IR_TYPE(ir) IT_##ir,
                FOR_IR(DECLARE_IR_TYPE)
                IT_INVALID
#undef DECLARE_IR_TYPE
            };

            virtual IrType getType() const = 0;

            virtual IrElement *visit(IrVisitor *const visitor) const = 0;

            virtual bool isAtom() const {
                return false;
            }

            virtual bool isLiteral() const {
                return false;
            }

            virtual bool isExpression() const {
                return false;
            }

            virtual Atom const *asAtom() const {
                return NULL;
            }

            virtual Expression const *asExpression() const {
                return NULL;
            }

#define HELPER(ir) virtual bool is##ir() const { return false; } ; virtual ir const* as##ir() const { return NULL; } ;

            FOR_IR(HELPER)
#undef HELPER

            virtual ~IrElement() {
            }
        };

        struct IrVisitor {
#define VISITORABSTR(ir) virtual IrElement *visit(const ir *const expr) { return NULL; }

            FOR_IR(VISITORABSTR)

#undef VISITORABSTR

#define VISITOR(ir) virtual IrElement *visit(const ir * const expr) ;

            virtual ~IrVisitor() {
            }
        };


        struct Expression : IrElement {
            virtual bool isExpression() const {
                return true;
            }

            virtual Expression const *asExpression() const {
                return this;
            }

            virtual ~Expression() {
            }
        };

        struct Atom : Expression {
            virtual bool isAtom() const {
                return true;
            }

            virtual const Atom *asAtom() const {
                return this;
            }

            virtual ~Atom() {
            }
        };

        enum VarType {
            VT_Undefined,
            VT_Unit,
            VT_Int,
            VT_Double,
            VT_Ptr,
            VT_Error
        };

        struct Variable : Atom {
            Variable(VarId id) : id(id) {
            }

            const VarId id;

            IR_COMMON_FUNCTIONS(Variable)
        };

        struct Statement : IrElement {
            virtual ~Statement() {
            }
        };

        struct Jump : IrElement {
            virtual ~Jump() {
            }
        };


        struct JumpAlways : Jump {
            Block *const destination;

            JumpAlways(Block *dest) : destination(dest) {
            }

            IR_COMMON_FUNCTIONS(JumpAlways)


            virtual ~JumpAlways() {
            }
        };

        struct JumpCond : Jump {
            JumpCond(Block *const yes, Block *const no, Atom const *const condition)
                    : yes(yes), no(no), condition(condition) {
            }

            Block *const yes;
            Block *const no;
            const Atom *const condition;


            virtual ~JumpCond() {
                delete condition;
            }

            IR_COMMON_FUNCTIONS(JumpCond)
        };

        struct Assignment : Statement {
            const Variable *const var;
            const Expression *const value;

            Assignment(Variable const *const var, Expression const *const expr) : var(var), value(expr) {
            }

            Assignment(VarId id, Expression const *const expr) : var(new Variable(id)), value(expr) {
            }

            IR_COMMON_FUNCTIONS(Assignment)

            virtual ~Assignment() {
                delete var;
                delete value;
            }
        };

        struct Return : Statement {
            Return(const Atom *const atom) : atom(atom) {
            }

            const Atom *const atom;

            IR_COMMON_FUNCTIONS(Return)

            virtual ~Return() {
                delete atom;
            }
        };

        struct Phi : Statement {
            const Variable *const var;

            Phi(VarId id) : var(new Variable(id)) {
            }

            Phi(Variable const *id) : var(id) {
            }

            std::set<const Variable *> vars;

            IR_COMMON_FUNCTIONS(Phi)


            virtual ~Phi() {
                delete var;
                for (auto v : vars) delete v;
            }
        };

        struct Call : Expression {
            Call(uint16_t id, std::vector<Atom const *> const &args, std::vector<VarId> const &refArgs
            )
                    : funId(id), params(args), refParams(refArgs) {
            }

            const uint16_t funId;
            std::vector<Atom const *> params;
            std::vector<VarId> refParams;


            IR_COMMON_FUNCTIONS(Call)

            virtual ~Call() {
                for (auto p : params) delete p;
            }
        };

        struct BinOp : Expression {

            const Atom *const left;
            const Atom *const right;
#define FOR_IR_BINOP(DO) \
DO(ADD, "+")\
DO(SUB, "-")\
DO(MUL, "*")\
DO(DIV, "/")\
DO(MOD, "%")\
DO(LT, "<")\
DO(LE, "<=")\
DO(EQ, "==")\
DO(NEQ, "!=")\
DO(OR, "|")\
DO(AND, "&")\
DO(LOR, "||")\
DO(LAND, "&&")\
DO(XOR, "^")

#define BINOP_ENUM(name, _) BO_##name,
            enum Type {
                FOR_IR_BINOP(BINOP_ENUM)
                BO_INVALID
            };
            const Type type;

            BinOp(Atom const *left, Atom const *right, Type type) : left(left), right(right), type(type) {
            }

            IR_COMMON_FUNCTIONS(BinOp)

            virtual ~BinOp() {
                delete left;
                delete right;
            }
        };

        struct UnOp : Expression {
            const Atom *const operand;
#define FOR_IR_UNOP(DO)\
DO(CAST_I2D, "<i2d>")\
DO(CAST_D2I,"<d2i>")\
DO(NEG, "-")\
DO(NOT, "!")
#define UNOP_NAME(o, _) UO_##o,

            enum Type {
                FOR_IR_UNOP(UNOP_NAME)
                UO_INVALID
            };
#undef UNOP_NAME

            const Type type;

            UnOp(const Atom *const operand, Type type) : operand(operand), type(type) {
            }

            IR_COMMON_FUNCTIONS(UnOp)


            virtual ~UnOp() {
                delete operand;
            }
        };

        struct Int : Atom {
            const int64_t value;

            virtual bool isLiteral() const {
                return true;
            }

            Int(int64_t value) : value(value) {
            }

            IR_COMMON_FUNCTIONS(Int)
        };

        struct Double : Atom {
            const double value;

            virtual bool isLiteral() const {
                return true;
            }

            Double(double value) : value(value) {
            }

            IR_COMMON_FUNCTIONS(Double)
        };

        struct Ptr : Atom {
            const VarId value;
            const bool isPooledString;

            virtual bool isLiteral() const {
                return true;
            }

            Ptr(VarId value, bool isPooledString) : value(value), isPooledString(isPooledString) {
            }

            IR_COMMON_FUNCTIONS(Ptr)
        };


        struct Block : IrElement {
        private:
            Jump *_transition;
        public:
            const std::string name;

            Jump *getTransition() const {
                return _transition;
            }

            void setTransition(Jump *jmp) {
                _transition = jmp;
                if (!jmp) return;
                if (jmp->isJumpAlways())
                    jmp->asJumpAlways()->destination->addPredecessor(this);
                else {
                    jmp->asJumpCond()->yes->addPredecessor(this);
                    jmp->asJumpCond()->no->addPredecessor(this);
                }
            }


            std::set<const Block *> predecessors;

            Block(std::string const &name) : _transition(NULL), name(name) {
            }


            void link(JumpCond *cond) {
                _transition = cond;
                cond->yes->addPredecessor(this);
                cond->no->addPredecessor(this);
            }

            void link(Block *next) {
                if (_transition) delete _transition;
                _transition = new JumpAlways(next);
                next->addPredecessor(this);
            }

            std::deque<Statement const *> contents;


            virtual ~Block() {
                for (auto s : contents)
                    delete s;
                delete _transition;
            }

            void addPredecessor(Block const *block) {
                predecessors.insert(block);
            }

            IR_COMMON_FUNCTIONS(Block)
        };


        class FunctionRecord : public IrElement {

        public:
            FunctionRecord(uint16_t id, VarType returnType)
                    : id(id), entry(new Block(mathvm::toString(id))), returnType(returnType) {
            }

            FunctionRecord(uint16_t id, VarType returnType, Block *startBlock)
                    : id(id), entry(startBlock), returnType(returnType) {
            }

            const uint16_t id;
            Block *entry;
            std::vector<VarId> parametersIds;
            std::vector<VarId> refParameterIds;
            std::vector<VarId> memoryCells; //each id is the pointer id. Write to it == write to this memory cell.
            // read from it == read from cell
            // pass it == pass the cell's address

            VarType returnType;

            IR_COMMON_FUNCTIONS(FunctionRecord)


            virtual ~FunctionRecord();
        };


        struct Print : Statement {
            Print(Atom const *const atom) : atom(atom) {
            }

            const Atom *const atom;

            IR_COMMON_FUNCTIONS(Print)

            virtual ~Print() {
                delete atom;
            }
        };

        struct WriteRef : Statement {
            WriteRef(Atom const *const atom, VarId const where) : atom(atom), refId(where) {
            }

            const Atom *const atom;
            const VarId refId;

            IR_COMMON_FUNCTIONS(WriteRef)

            virtual ~WriteRef() {
                delete atom;
            }
        };

        struct ReadRef : Atom {
            const VarId refId;

            IR_COMMON_FUNCTIONS(ReadRef)

            ReadRef(VarId const refId) : refId(refId) {
            }

        };

        struct SimpleIr {

            struct VarMeta {
                const VarId id;
                const bool isSourceVar;
                const bool isReference;
                const VarId originId;
                const FunctionRecord *pointsTo;
                const uint32_t offset;

                VarType type;

                VarMeta(VarId id, VarId from, VarType type)
                        : id(id),
                          isSourceVar(true),
                          isReference(false),
                          originId(from),
                          pointsTo(NULL),
                          offset(0),
                          type(type) {
                }

                VarMeta(VarMeta const &meta) : id(meta.id),
                                               isSourceVar(meta.isSourceVar),
                                               isReference(meta.isReference),
                                               originId(meta.originId),
                                               pointsTo(meta.pointsTo),
                                               offset(meta.offset),
                                               type(meta.type) {
                }

                VarMeta(VarId id)
                        : id(id),
                          isSourceVar(false),
                          isReference(false),
                          originId(0),
                          pointsTo(NULL),
                          offset(0),
                          type(VT_Undefined) {
                }

                VarMeta(VarId id, VarType type, FunctionRecord const *pointsTo, uint32_t offset)
                        : id(id),
                          isSourceVar(false),
                          isReference(true),
                          originId(0),
                          pointsTo(pointsTo),
                          offset(offset),
                          type(type) {
                }
            };

            typedef std::vector<std::string> StringPool;

            StringPool pool;
            std::vector<FunctionRecord *> functions;

            void addFunction(FunctionRecord *rec) {
                functions.push_back(rec);
            }

            std::vector<VarMeta> varMeta;

            virtual ~SimpleIr() {
                for (auto f : functions)
                    delete f;
            }
        };

        typedef SimpleIr SimpleSsaIr;


    }
}