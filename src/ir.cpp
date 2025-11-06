#include "ir.h"
#include <sstream>

namespace pyplusplus {

// IRValue implementations
std::string IRConstant::toString() const {
    std::ostringstream oss;
    oss << getName() << " = ";
    
    if (isInt()) {
        oss << asInt();
    } else if (isFloat()) {
        oss << asFloat();
    } else if (isString()) {
        oss << "\"" << asString() << "\"";
    } else if (isBool()) {
        oss << (asBool() ? "True" : "False");
    } else if (isNone()) {
        oss << "None";
    }
    
    oss << " : " << getType()->toString();
    return oss.str();
}

std::string IRParameter::toString() const {
    return getName() + " : " + getType()->toString();
}

std::string IRLocal::toString() const {
    return getName() + " : " + getType()->toString();
}

std::string IRGlobal::toString() const {
    return getName() + " : " + getType()->toString();
}

std::string IRTemporary::toString() const {
    return getName() + " : " + getType()->toString();
}

// IRInstruction implementations
std::string IRBinaryOp::toString() const {
    std::ostringstream oss;
    oss << "t" << getId() << " = ";
    
    switch (getOp()) {
        case IROp::ADD: oss << "add"; break;
        case IROp::SUB: oss << "sub"; break;
        case IROp::MUL: oss << "mul"; break;
        case IROp::DIV: oss << "div"; break;
        case IROp::MOD: oss << "mod"; break;
        case IROp::POW: oss << "pow"; break;
        case IROp::EQ: oss << "eq"; break;
        case IROp::NE: oss << "ne"; break;
        case IROp::LT: oss << "lt"; break;
        case IROp::LE: oss << "le"; break;
        case IROp::GT: oss << "gt"; break;
        case IROp::GE: oss << "ge"; break;
        case IROp::AND: oss << "and"; break;
        case IROp::OR: oss << "or"; break;
        case IROp::BIT_AND: oss << "bit_and"; break;
        case IROp::BIT_OR: oss << "bit_or"; break;
        case IROp::BIT_XOR: oss << "bit_xor"; break;
        case IROp::LEFT_SHIFT: oss << "left_shift"; break;
        case IROp::RIGHT_SHIFT: oss << "right_shift"; break;
        default: oss << "unknown"; break;
    }
    
    oss << " " << getOperands()[0]->getName() << ", " << getOperands()[1]->getName();
    if (getResultType()) {
        oss << " : " << getResultType()->toString();
    }
    
    return oss.str();
}

std::string IRUnaryOp::toString() const {
    std::ostringstream oss;
    oss << "t" << getId() << " = ";
    
    switch (getOp()) {
        case IROp::NOT: oss << "not"; break;
        case IROp::BIT_NOT: oss << "bit_not"; break;
        default: oss << "unknown"; break;
    }
    
    oss << " " << getOperands()[0]->getName();
    if (getResultType()) {
        oss << " : " << getResultType()->toString();
    }
    
    return oss.str();
}

std::string IRCall::toString() const {
    std::ostringstream oss;
    oss << "t" << getId() << " = call " << getCallee()->getName() << "(";
    
    auto args = getArguments();
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << args[i]->getName();
    }
    
    oss << ")";
    if (getResultType()) {
        oss << " : " << getResultType()->toString();
    }
    
    return oss.str();
}

std::string IRReturn::toString() const {
    std::ostringstream oss;
    oss << "return";
    if (getValue()) {
        oss << " " << getValue()->getName();
    }
    return oss.str();
}

std::string IRJump::toString() const {
    return "jump " + target->getName();
}

std::string IRJumpIf::toString() const {
    return "jump_if " + getCondition()->getName() + " to " + true_target->getName() + 
           " else " + false_target->getName();
}

std::string IRLoad::toString() const {
    return "t" + std::to_string(getId()) + " = load " + getAddress()->getName();
}

std::string IRStore::toString() const {
    return "store " + getValue()->getName() + " to " + getAddress()->getName();
}

std::string IRAlloc::toString() const {
    return "t" + std::to_string(getId()) + " = alloc " + getResultType()->toString();
}

// IRBasicBlock implementation
void IRBasicBlock::addInstruction(std::unique_ptr<IRInstruction> instruction) {
    instructions.push_back(std::move(instruction));
}

std::string IRBasicBlock::toString() const {
    std::ostringstream oss;
    oss << name << ":\n";
    
    for (const auto& instruction : instructions) {
        oss << "  " << instruction->toString() << "\n";
    }
    
    return oss.str();
}

// IRFunction implementation
std::shared_ptr<IRBasicBlock> IRFunction::createBasicBlock(const std::string& name) {
    auto block = std::make_shared<IRBasicBlock>(name);
    basic_blocks.push_back(std::make_unique<IRBasicBlock>(name));
    return block;
}

std::shared_ptr<IRValue> IRFunction::createConstant(int value, std::shared_ptr<Type> type) {
    auto constant = std::make_shared<IRConstant>(value, std::move(type), "const_" + std::to_string(value));
    values[constant->getName()] = constant;
    return constant;
}

std::shared_ptr<IRValue> IRFunction::createConstant(double value, std::shared_ptr<Type> type) {
    auto constant = std::make_shared<IRConstant>(value, std::move(type), "const_" + std::to_string(value));
    values[constant->getName()] = constant;
    return constant;
}

std::shared_ptr<IRValue> IRFunction::createConstant(const std::string& value, std::shared_ptr<Type> type) {
    auto constant = std::make_shared<IRConstant>(value, std::move(type), "const_str");
    values[constant->getName()] = constant;
    return constant;
}

std::shared_ptr<IRValue> IRFunction::createConstant(bool value, std::shared_ptr<Type> type) {
    auto constant = std::make_shared<IRConstant>(value, std::move(type), value ? "True" : "False");
    values[constant->getName()] = constant;
    return constant;
}

std::shared_ptr<IRValue> IRFunction::createConstant(std::nullptr_t, std::shared_ptr<Type> type) {
    auto constant = std::make_shared<IRConstant>(nullptr, std::move(type), "None");
    values[constant->getName()] = constant;
    return constant;
}

std::shared_ptr<IRValue> IRFunction::createTemporary(std::shared_ptr<Type> type) {
    auto temp = std::make_shared<IRTemporary>(next_temp_id++, std::move(type));
    values[temp->getName()] = temp;
    return temp;
}

std::string IRFunction::toString() const {
    std::ostringstream oss;
    oss << "function " << name << "(";
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters[i]->toString();
    }
    
    oss << ") : " << type->getReturnType()->toString() << " {\n";
    
    for (const auto& block : basic_blocks) {
        std::string block_str = block->toString();
        for (char c : block_str) {
            oss << c;
            if (c == '\n') oss << "  ";
        }
    }
    
    oss << "}\n";
    return oss.str();
}

// IRModule implementation
std::shared_ptr<IRFunction> IRModule::createFunction(const std::string& name, std::shared_ptr<FunctionType> type) {
    auto function = std::make_shared<IRFunction>(name, std::move(type));
    functions.push_back(std::make_unique<IRFunction>(name, function->getType()));
    return function;
}

std::shared_ptr<IRGlobal> IRModule::createGlobal(const std::string& name, std::shared_ptr<Type> type) {
    auto global = std::make_shared<IRGlobal>(std::move(type), name);
    globals[name] = global;
    return global;
}

std::string IRModule::toString() const {
    std::ostringstream oss;
    oss << "module " << name << " {\n\n";
    
    // Globals
    for (const auto& [name, global] : globals) {
        oss << "  global " << global->toString() << "\n";
    }
    
    oss << "\n";
    
    // Functions
    for (const auto& function : functions) {
        std::string func_str = function->toString();
        for (char c : func_str) {
            oss << c;
            if (c == '\n') oss << "  ";
        }
        oss << "\n";
    }
    
    oss << "}\n";
    return oss.str();
}

// IRBuilder implementation
std::shared_ptr<IRValue> IRBuilder::createConstant(int value, std::shared_ptr<Type> type) {
    return current_function->createConstant(value, std::move(type));
}

std::shared_ptr<IRValue> IRBuilder::createConstant(double value, std::shared_ptr<Type> type) {
    return current_function->createConstant(value, std::move(type));
}

std::shared_ptr<IRValue> IRBuilder::createConstant(const std::string& value, std::shared_ptr<Type> type) {
    return current_function->createConstant(value, std::move(type));
}

std::shared_ptr<IRValue> IRBuilder::createConstant(bool value, std::shared_ptr<Type> type) {
    return current_function->createConstant(value, std::move(type));
}

std::shared_ptr<IRValue> IRBuilder::createConstant(std::nullptr_t, std::shared_ptr<Type> type) {
    return current_function->createConstant(nullptr, std::move(type));
}

std::shared_ptr<IRValue> IRBuilder::createTemporary(std::shared_ptr<Type> type) {
    return current_function->createTemporary(std::move(type));
}

std::shared_ptr<IRValue> IRBuilder::createBinaryOp(IROp op, std::shared_ptr<IRValue> left, std::shared_ptr<IRValue> right) {
    // Determine result type
    std::shared_ptr<Type> result_type;
    // TODO: Implement proper type inference for binary operations
    
    auto instruction = std::make_unique<IRBinaryOp>(op, result_type, std::move(left), std::move(right));
    auto result = createTemporary(result_type);
    current_block->addInstruction(std::move(instruction));
    return result;
}

std::shared_ptr<IRValue> IRBuilder::createUnaryOp(IROp op, std::shared_ptr<IRValue> operand) {
    // Determine result type
    std::shared_ptr<Type> result_type;
    // TODO: Implement proper type inference for unary operations
    
    auto instruction = std::make_unique<IRUnaryOp>(op, result_type, std::move(operand));
    auto result = createTemporary(result_type);
    current_block->addInstruction(std::move(instruction));
    return result;
}

std::shared_ptr<IRValue> IRBuilder::createCall(std::shared_ptr<IRValue> callee, const std::vector<std::shared_ptr<IRValue>>& args) {
    // Determine result type
    std::shared_ptr<Type> result_type;
    if (callee->getType()->getKind() == Type::Kind::FUNCTION) {
        auto func_type = static_cast<const FunctionType*>(callee->getType());
        result_type = std::shared_ptr<Type>(const_cast<Type*>(func_type->getReturnType()), [](Type*){});
    }
    
    auto instruction = std::make_unique<IRCall>(result_type, std::move(callee), args);
    auto result = createTemporary(result_type);
    current_block->addInstruction(std::move(instruction));
    return result;
}

std::shared_ptr<IRValue> IRBuilder::createLoad(std::shared_ptr<IRValue> address) {
    auto instruction = std::make_unique<IRLoad>(std::move(address));
    auto result = createTemporary(instruction->getResultType());
    current_block->addInstruction(std::move(instruction));
    return result;
}

std::shared_ptr<IRValue> IRBuilder::createAlloc(std::shared_ptr<Type> type, const std::string& name) {
    auto instruction = std::make_unique<IRAlloc>(std::move(type), name);
    auto result = createTemporary(type);
    current_block->addInstruction(std::move(instruction));
    return result;
}

void IRBuilder::createStore(std::shared_ptr<IRValue> value, std::shared_ptr<IRValue> address) {
    auto instruction = std::make_unique<IRStore>(std::move(value), std::move(address));
    current_block->addInstruction(std::move(instruction));
}

void IRBuilder::createReturn(std::shared_ptr<IRValue> value) {
    auto instruction = std::make_unique<IRReturn>(std::move(value));
    current_block->addInstruction(std::move(instruction));
}

void IRBuilder::createJump(std::shared_ptr<IRBasicBlock> target) {
    auto instruction = std::make_unique<IRJump>(std::move(target));
    current_block->addInstruction(std::move(instruction));
}

void IRBuilder::createJumpIf(std::shared_ptr<IRValue> condition, std::shared_ptr<IRBasicBlock> true_target, std::shared_ptr<IRBasicBlock> false_target) {
    auto instruction = std::make_unique<IRJumpIf>(std::move(condition), std::move(true_target), std::move(false_target));
    current_block->addInstruction(std::move(instruction));
}

} // namespace pyplusplus