#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include "types.h"

namespace pyplusplus {

// Forward declarations
class IRValue;
class IRInstruction;
class IRBasicBlock;
class IRFunction;
class IRModule;

// IR Value types
enum class IRValueType {
    CONSTANT,
    PARAMETER,
    LOCAL,
    GLOBAL,
    TEMPORARY
};

// IR Operations
enum class IROp {
    // Arithmetic
    ADD, SUB, MUL, DIV, MOD, POW,
    
    // Comparison
    EQ, NE, LT, LE, GT, GE,
    
    // Logical
    AND, OR, NOT,
    
    // Bitwise
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, LEFT_SHIFT, RIGHT_SHIFT,
    
    // Control flow
    CALL, RETURN, JUMP, JUMP_IF_TRUE, JUMP_IF_FALSE,
    
    // Memory
    LOAD, STORE, ALLOC,
    
    // Conversions
    CAST,
    
    // Python-specific
    GET_ATTR, SET_ATTR, GET_SUBSCRIPT, SET_SUBSCRIPT,
    LIST_CREATE, DICT_CREATE, FUNCTION_CREATE,
    
    // Special
    PHI
};

// Base IR value class
class IRValue {
private:
    IRValueType value_type;
    std::shared_ptr<Type> type;
    std::string name;
    
public:
    IRValue(IRValueType value_type, std::shared_ptr<Type> type, const std::string& name)
        : value_type(value_type), type(std::move(type)), name(name) {}
    
    virtual ~IRValue() = default;
    
    IRValueType getValueType() const { return value_type; }
    const Type* getType() const { return type.get(); }
    const std::string& getName() const { return name; }
    
    virtual std::string toString() const = 0;
};

// Constant value
class IRConstant : public IRValue {
private:
    std::variant<int, double, std::string, bool, std::nullptr_t> value;
    
public:
    IRConstant(int value, std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::CONSTANT, std::move(type), name), value(value) {}
    
    IRConstant(double value, std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::CONSTANT, std::move(type), name), value(value) {}
    
    IRConstant(const std::string& value, std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::CONSTANT, std::move(type), name), value(value) {}
    
    IRConstant(bool value, std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::CONSTANT, std::move(type), name), value(value) {}
    
    IRConstant(std::nullptr_t, std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::CONSTANT, std::move(type), name), value(nullptr) {}
    
    std::string toString() const override;
    
    const auto& getValue() const { return value; }
    bool isInt() const { return std::holds_alternative<int>(value); }
    bool isFloat() const { return std::holds_alternative<double>(value); }
    bool isString() const { return std::holds_alternative<std::string>(value); }
    bool isBool() const { return std::holds_alternative<bool>(value); }
    bool isNone() const { return std::holds_alternative<std::nullptr_t>(value); }
    
    int asInt() const { return std::get<int>(value); }
    double asFloat() const { return std::get<double>(value); }
    const std::string& asString() const { return std::get<std::string>(value); }
    bool asBool() const { return std::get<bool>(value); }
};

// Parameter value
class IRParameter : public IRValue {
private:
    int index;
    
public:
    IRParameter(int index, std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::PARAMETER, std::move(type), name), index(index) {}
    
    std::string toString() const override;
    int getIndex() const { return index; }
};

// Local variable
class IRLocal : public IRValue {
public:
    IRLocal(std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::LOCAL, std::move(type), name) {}
    
    std::string toString() const override;
};

// Global variable
class IRGlobal : public IRValue {
public:
    IRGlobal(std::shared_ptr<Type> type, const std::string& name)
        : IRValue(IRValueType::GLOBAL, std::move(type), name) {}
    
    std::string toString() const override;
};

// Temporary value (result of instruction)
class IRTemporary : public IRValue {
private:
    int id;
    
public:
    IRTemporary(int id, std::shared_ptr<Type> type)
        : IRValue(IRValueType::TEMPORARY, std::move(type), "t" + std::to_string(id)), id(id) {}
    
    std::string toString() const override;
    int getId() const { return id; }
};

// IR Instruction base class
class IRInstruction {
private:
    IROp op;
    std::shared_ptr<Type> result_type;
    std::vector<std::shared_ptr<IRValue>> operands;
    std::string comment;
    
public:
    IRInstruction(IROp op, std::shared_ptr<Type> result_type, 
                  std::vector<std::shared_ptr<IRValue>> operands, const std::string& comment = "")
        : op(op), result_type(std::move(result_type)), operands(std::move(operands)), comment(comment) {}
    
    virtual ~IRInstruction() = default;
    
    IROp getOp() const { return op; }
    const Type* getResultType() const { return result_type.get(); }
    const std::vector<std::shared_ptr<IRValue>>& getOperands() const { return operands; }
    const std::string& getComment() const { return comment; }
    
    virtual std::string toString() const = 0;
};

// Binary operation instruction
class IRBinaryOp : public IRInstruction {
public:
    IRBinaryOp(IROp op, std::shared_ptr<Type> result_type, 
                std::shared_ptr<IRValue> left, std::shared_ptr<IRValue> right)
        : IRInstruction(op, std::move(result_type), {std::move(left), std::move(right)}) {}
    
    std::string toString() const override;
};

// Unary operation instruction
class IRUnaryOp : public IRInstruction {
public:
    IRUnaryOp(IROp op, std::shared_ptr<Type> result_type, std::shared_ptr<IRValue> operand)
        : IRInstruction(op, std::move(result_type), {std::move(operand)}) {}
    
    std::string toString() const override;
};

// Call instruction
class IRCall : public IRInstruction {
public:
    IRCall(std::shared_ptr<Type> result_type, std::shared_ptr<IRValue> callee,
            std::vector<std::shared_ptr<IRValue>> arguments)
        : IRInstruction(IROp::CALL, std::move(result_type), 
                       [&]() {
                           std::vector<std::shared_ptr<IRValue>> ops = {std::move(callee)};
                           ops.insert(ops.end(), std::make_move_iterator(arguments.begin()), 
                                     std::make_move_iterator(arguments.end()));
                           return ops;
                       }()) {}
    
    std::string toString() const override;
    IRValue* getCallee() const { return getOperands()[0].get(); }
    std::vector<std::shared_ptr<IRValue>> getArguments() const {
        auto args = getOperands();
        args.erase(args.begin()); // Remove callee
        return args;
    }
};

// Return instruction
class IRReturn : public IRInstruction {
public:
    explicit IRReturn(std::shared_ptr<IRValue> value = nullptr)
        : IRInstruction(IROp::RETURN, nullptr, value ? std::vector<std::shared_ptr<IRValue>>{value} : std::vector<std::shared_ptr<IRValue>>{}) {}
    
    std::string toString() const override;
    IRValue* getValue() const { return getOperands().empty() ? nullptr : getOperands()[0].get(); }
};

// Jump instruction
class IRJump : public IRInstruction {
private:
    std::shared_ptr<IRBasicBlock> target;
    
public:
    explicit IRJump(std::shared_ptr<IRBasicBlock> target)
        : IRInstruction(IROp::JUMP, nullptr, {}), target(std::move(target)) {}
    
    std::string toString() const override;
    IRBasicBlock* getTarget() const { return target.get(); }
};

// Conditional jump instruction
class IRJumpIf : public IRInstruction {
private:
    std::shared_ptr<IRBasicBlock> true_target;
    std::shared_ptr<IRBasicBlock> false_target;
    
public:
    IRJumpIf(std::shared_ptr<IRValue> condition, std::shared_ptr<IRBasicBlock> true_target, 
              std::shared_ptr<IRBasicBlock> false_target)
        : IRInstruction(IROp::JUMP_IF_TRUE, nullptr, {std::move(condition)}),
          true_target(std::move(true_target)), false_target(std::move(false_target)) {}
    
    std::string toString() const override;
    IRValue* getCondition() const { return getOperands()[0].get(); }
    IRBasicBlock* getTrueTarget() const { return true_target.get(); }
    IRBasicBlock* getFalseTarget() const { return false_target.get(); }
};

// Load instruction
class IRLoad : public IRInstruction {
public:
    explicit IRLoad(std::shared_ptr<IRValue> address)
        : IRInstruction(IROp::LOAD, address->getType()->getKind() == Type::Kind::POINTER ? 
                       std::shared_ptr<Type>(const_cast<Type*>(static_cast<const PointerType*>(address->getType())->getElementType()), [](Type*){}) : 
                       std::shared_ptr<Type>(), {std::move(address)}) {}
    
    std::string toString() const override;
    IRValue* getAddress() const { return getOperands()[0].get(); }
};

// Store instruction
class IRStore : public IRInstruction {
public:
    IRStore(std::shared_ptr<IRValue> value, std::shared_ptr<IRValue> address)
        : IRInstruction(IROp::STORE, nullptr, {std::move(value), std::move(address)}) {}
    
    std::string toString() const override;
    IRValue* getValue() const { return getOperands()[0].get(); }
    IRValue* getAddress() const { return getOperands()[1].get(); }
};

// Allocation instruction
class IRAlloc : public IRInstruction {
public:
    IRAlloc(std::shared_ptr<Type> type, const std::string& name)
        : IRInstruction(IROp::ALLOC, type, {}, name) {}
    
    std::string toString() const override;
};

// Basic block
class IRBasicBlock {
private:
    std::string name;
    std::vector<std::unique_ptr<IRInstruction>> instructions;
    std::vector<std::shared_ptr<IRBasicBlock>> predecessors;
    std::vector<std::shared_ptr<IRBasicBlock>> successors;
    
public:
    explicit IRBasicBlock(const std::string& name) : name(name) {}
    
    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<IRInstruction>>& getInstructions() const { return instructions; }
    const std::vector<std::shared_ptr<IRBasicBlock>>& getPredecessors() const { return predecessors; }
    const std::vector<std::shared_ptr<IRBasicBlock>>& getSuccessors() const { return successors; }
    
    void addInstruction(std::unique_ptr<IRInstruction> instruction);
    void addPredecessor(std::shared_ptr<IRBasicBlock> pred) { predecessors.push_back(std::move(pred)); }
    void addSuccessor(std::shared_ptr<IRBasicBlock> succ) { successors.push_back(std::move(succ)); }
    
    std::string toString() const;
};

// Function
class IRFunction {
private:
    std::string name;
    std::shared_ptr<FunctionType> type;
    std::vector<std::shared_ptr<IRParameter>> parameters;
    std::vector<std::unique_ptr<IRBasicBlock>> basic_blocks;
    std::unordered_map<std::string, std::shared_ptr<IRValue>> values;
    int next_temp_id;
    
public:
    IRFunction(const std::string& name, std::shared_ptr<FunctionType> type)
        : name(name), type(std::move(type)), next_temp_id(0) {}
    
    const std::string& getName() const { return name; }
    const FunctionType* getType() const { return type.get(); }
    const std::vector<std::shared_ptr<IRParameter>>& getParameters() const { return parameters; }
    const std::vector<std::unique_ptr<IRBasicBlock>>& getBasicBlocks() const { return basic_blocks; }
    
    void addParameter(std::shared_ptr<IRParameter> param) { parameters.push_back(std::move(param)); }
    std::shared_ptr<IRBasicBlock> createBasicBlock(const std::string& name);
    std::shared_ptr<IRValue> createConstant(int value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(double value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(const std::string& value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(bool value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(std::nullptr_t, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createTemporary(std::shared_ptr<Type> type);
    
    std::string toString() const;
};

// Module
class IRModule {
private:
    std::string name;
    std::vector<std::unique_ptr<IRFunction>> functions;
    std::unordered_map<std::string, std::shared_ptr<IRGlobal>> globals;
    
public:
    explicit IRModule(const std::string& name) : name(name) {}
    
    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<IRFunction>>& getFunctions() const { return functions; }
    const std::unordered_map<std::string, std::shared_ptr<IRGlobal>>& getGlobals() const { return globals; }
    
    std::shared_ptr<IRFunction> createFunction(const std::string& name, std::shared_ptr<FunctionType> type);
    std::shared_ptr<IRGlobal> createGlobal(const std::string& name, std::shared_ptr<Type> type);
    
    std::string toString() const;
};

// IR Builder - helper class for building IR
class IRBuilder {
private:
    IRFunction* current_function;
    IRBasicBlock* current_block;
    
public:
    explicit IRBuilder(IRFunction* function = nullptr, IRBasicBlock* block = nullptr)
        : current_function(function), current_block(block) {}
    
    void setFunction(IRFunction* function) { current_function = function; }
    void setBlock(IRBasicBlock* block) { current_block = block; }
    
    // Value creation
    std::shared_ptr<IRValue> createConstant(int value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(double value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(const std::string& value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(bool value, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createConstant(std::nullptr_t, std::shared_ptr<Type> type);
    std::shared_ptr<IRValue> createTemporary(std::shared_ptr<Type> type);
    
    // Instruction creation
    std::shared_ptr<IRValue> createBinaryOp(IROp op, std::shared_ptr<IRValue> left, std::shared_ptr<IRValue> right);
    std::shared_ptr<IRValue> createUnaryOp(IROp op, std::shared_ptr<IRValue> operand);
    std::shared_ptr<IRValue> createCall(std::shared_ptr<IRValue> callee, const std::vector<std::shared_ptr<IRValue>>& args);
    std::shared_ptr<IRValue> createLoad(std::shared_ptr<IRValue> address);
    std::shared_ptr<IRValue> createAlloc(std::shared_ptr<Type> type, const std::string& name = "");
    void createStore(std::shared_ptr<IRValue> value, std::shared_ptr<IRValue> address);
    void createReturn(std::shared_ptr<IRValue> value = nullptr);
    void createJump(std::shared_ptr<IRBasicBlock> target);
    void createJumpIf(std::shared_ptr<IRValue> condition, std::shared_ptr<IRBasicBlock> true_target, std::shared_ptr<IRBasicBlock> false_target);
};

} // namespace pyplusplus