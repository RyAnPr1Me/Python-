#include "bytecode.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace pyplusplus {

// BytecodeTranslator implementation
BytecodeTranslator::BytecodeTranslator(TypeSystem& type_system) 
    : type_system(type_system) {
    ir_module = std::make_unique<IRModule>("translated");
}

std::unique_ptr<IRModule> BytecodeTranslator::translate(const std::vector<BytecodeInstruction>& bytecode, 
                                                        const std::string& module_name) {
    reset();
    ir_module = std::make_unique<IRModule>(module_name);
    
    // Create main function
    auto func_type = type_system.createFunctionType({}, type_system.getAnyType());
    current_function = std::make_unique<IRFunction>("__main__", func_type);
    
    // Create entry block
    current_block = std::make_unique<IRBasicBlock>("entry");
    block_map[0] = std::make_unique<IRBasicBlock>("entry");
    
    // First pass: identify all basic blocks
    for (const auto& instr : bytecode) {
        if (isJumpInstruction(instr.opcode)) {
            uint32_t target = getJumpTarget(instr);
            if (block_map.find(target) == block_map.end()) {
                block_map[target] = std::make_unique<IRBasicBlock>("block_" + std::to_string(target));
            }
        }
    }
    
    // Second pass: translate instructions
    for (const auto& instr : bytecode) {
        setCurrentBlock(block_map[instr.offset].get());
        translateInstruction(instr);
    }
    
    // Add function to module
    ir_module->addFunction(std::move(current_function));
    
    return std::move(ir_module);
}

void BytecodeTranslator::translateInstruction(const BytecodeInstruction& instr) {
    switch (static_cast<BytecodeOpcode>(instr.opcode)) {
        case BytecodeOpcode::LOAD_CONST:
            translateLoadConst(instr);
            break;
        case BytecodeOpcode::LOAD_FAST:
            translateLoadFast(instr);
            break;
        case BytecodeOpcode::STORE_FAST:
            translateStoreFast(instr);
            break;
        case BytecodeOpcode::LOAD_GLOBAL:
            translateLoadGlobal(instr);
            break;
        case BytecodeOpcode::STORE_GLOBAL:
            translateStoreGlobal(instr);
            break;
        case BytecodeOpcode::BINARY_ADD:
        case BytecodeOpcode::BINARY_SUBTRACT:
        case BytecodeOpcode::BINARY_MULTIPLY:
        case BytecodeOpcode::BINARY_TRUE_DIVIDE:
        case BytecodeOpcode::BINARY_MODULO:
        case BytecodeOpcode::BINARY_POWER:
            translateBinaryOp(instr);
            break;
        case BytecodeOpcode::UNARY_POSITIVE:
        case BytecodeOpcode::UNARY_NEGATIVE:
        case BytecodeOpcode::UNARY_NOT:
        case BytecodeOpcode::UNARY_INVERT:
            translateUnaryOp(instr);
            break;
        case BytecodeOpcode::COMPARE_OP:
            translateCompareOp(instr);
            break;
        case BytecodeOpcode::JUMP_FORWARD:
        case BytecodeOpcode::JUMP_ABSOLUTE:
            translateJump(instr);
            break;
        case BytecodeOpcode::POP_JUMP_IF_TRUE:
        case BytecodeOpcode::POP_JUMP_IF_FALSE:
        case BytecodeOpcode::JUMP_IF_TRUE_OR_POP:
        case BytecodeOpcode::JUMP_IF_FALSE_OR_POP:
            translateJumpIf(instr);
            break;
        case BytecodeOpcode::CALL_FUNCTION:
        case BytecodeOpcode::CALL_FUNCTION_KW:
        case BytecodeOpcode::CALL_FUNCTION_EX:
            translateCallFunction(instr);
            break;
        case BytecodeOpcode::BUILD_TUPLE:
            translateBuildTuple(instr);
            break;
        case BytecodeOpcode::BUILD_LIST:
            translateBuildList(instr);
            break;
        case BytecodeOpcode::BUILD_SET:
            translateBuildSet(instr);
            break;
        case BytecodeOpcode::BUILD_MAP:
            translateBuildMap(instr);
            break;
        case BytecodeOpcode::LOAD_ATTR:
            translateLoadAttr(instr);
            break;
        case BytecodeOpcode::STORE_ATTR:
            translateStoreAttr(instr);
            break;
        case BytecodeOpcode::LOAD_SUBSCR:
            translateLoadSubscr(instr);
            break;
        case BytecodeOpcode::STORE_SUBSCR:
            translateStoreSubscr(instr);
            break;
        case BytecodeOpcode::RETURN_VALUE:
            translateReturnValue(instr);
            break;
        case BytecodeOpcode::GET_ITER:
            translateGetIter(instr);
            break;
        case BytecodeOpcode::FOR_ITER:
            translateForIter(instr);
            break;
        case BytecodeOpcode::IMPORT_NAME:
            translateImportName(instr);
            break;
        case BytecodeOpcode::MAKE_FUNCTION:
            translateMakeFunction(instr);
            break;
        default:
            // Handle unknown or unsupported opcodes
            std::cerr << "Warning: Unsupported opcode " << static_cast<int>(instr.opcode) 
                      << " at offset " << instr.offset << std::endl;
            break;
    }
}

void BytecodeTranslator::translateLoadConst(const BytecodeInstruction& instr) {
    auto constant = getConstant(instr.arg);
    pushStack(constant);
}

void BytecodeTranslator::translateLoadFast(const BytecodeInstruction& instr) {
    auto local = getLocal(instr.arg);
    pushStack(local);
}

void BytecodeTranslator::translateStoreFast(const BytecodeInstruction& instr) {
    auto value = popStack();
    setLocal(instr.arg, value);
}

void BytecodeTranslator::translateLoadGlobal(const BytecodeInstruction& instr) {
    auto global = getGlobal(instr.arg);
    pushStack(global);
}

void BytecodeTranslator::translateStoreGlobal(const BytecodeInstruction& instr) {
    auto value = popStack();
    setGlobal(instr.arg, value);
}

void BytecodeTranslator::translateBinaryOp(const BytecodeInstruction& instr) {
    auto right = popStack();
    auto left = popStack();
    
    IROp op;
    switch (static_cast<BytecodeOpcode>(instr.opcode)) {
        case BytecodeOpcode::BINARY_ADD:
            op = IROp::ADD;
            break;
        case BytecodeOpcode::BINARY_SUBTRACT:
            op = IROp::SUB;
            break;
        case BytecodeOpcode::BINARY_MULTIPLY:
            op = IROp::MUL;
            break;
        case BytecodeOpcode::BINARY_TRUE_DIVIDE:
            op = IROp::DIV;
            break;
        case BytecodeOpcode::BINARY_MODULO:
            op = IROp::MOD;
            break;
        case BytecodeOpcode::BINARY_POWER:
            op = IROp::POW;
            break;
        default:
            op = IROp::ADD;  // Default
            break;
    }
    
    auto result_type = type_system.getAnyType();
    auto result = createTemporary(result_type);
    auto binary_op = std::make_unique<IRBinaryOp>(op, result_type, left, right);
    current_block->addInstruction(std::move(binary_op));
    pushStack(result);
}

void BytecodeTranslator::translateUnaryOp(const BytecodeInstruction& instr) {
    auto operand = popStack();
    
    IROp op;
    switch (static_cast<BytecodeOpcode>(instr.opcode)) {
        case BytecodeOpcode::UNARY_POSITIVE:
            op = IROp::ADD;  // Will be handled as unary + in optimization
            break;
        case BytecodeOpcode::UNARY_NEGATIVE:
            op = IROp::SUB;  // Will be handled as unary - in optimization
            break;
        case BytecodeOpcode::UNARY_NOT:
            op = IROp::NOT;
            break;
        case BytecodeOpcode::UNARY_INVERT:
            op = IROp::BIT_NOT;
            break;
        default:
            op = IROp::NOT;
            break;
    }
    
    auto result_type = type_system.getAnyType();
    auto result = createTemporary(result_type);
    auto unary_op = std::make_unique<IRUnaryOp>(op, result_type, operand);
    current_block->addInstruction(std::move(unary_op));
    pushStack(result);
}

void BytecodeTranslator::translateCompareOp(const BytecodeInstruction& instr) {
    auto right = popStack();
    auto left = popStack();
    
    IROp op;
    switch (instr.arg) {
        case 0:  // <
            op = IROp::LT;
            break;
        case 1:  // <=
            op = IROp::LE;
            break;
        case 2:  // ==
            op = IROp::EQ;
            break;
        case 3:  // !=
            op = IROp::NE;
            break;
        case 4:  // >
            op = IROp::GT;
            break;
        case 5:  // >=
            op = IROp::GE;
            break;
        default:
            op = IROp::EQ;
            break;
    }
    
    auto result_type = type_system.getAnyType();
    auto result = createTemporary(result_type);
    auto compare_op = std::make_unique<IRBinaryOp>(op, result_type, left, right);
    current_block->addInstruction(std::move(compare_op));
    pushStack(result);
}

void BytecodeTranslator::translateJump(const BytecodeInstruction& instr) {
    uint32_t target = getJumpTarget(instr);
    auto target_block = getOrCreateBlock(target);
    auto jump = std::make_unique<IRJump>(target_block);
    current_block->addInstruction(std::move(jump));
}

void BytecodeTranslator::translateJumpIf(const BytecodeInstruction& instr) {
    auto condition = popStack();
    uint32_t target = getJumpTarget(instr);
    auto target_block = getOrCreateBlock(target);
    auto fallthrough_block = getOrCreateBlock(instr.offset + 2);  // Next instruction
    
    bool jump_if_true = (static_cast<BytecodeOpcode>(instr.opcode) == BytecodeOpcode::POP_JUMP_IF_TRUE ||
                        static_cast<BytecodeOpcode>(instr.opcode) == BytecodeOpcode::JUMP_IF_TRUE_OR_POP);
    
    auto jump_if = std::make_unique<IRJumpIf>(condition, 
                                              jump_if_true ? target_block : fallthrough_block,
                                              jump_if_true ? fallthrough_block : target_block);
    current_block->addInstruction(std::move(jump_if));
}

void BytecodeTranslator::translateCallFunction(const BytecodeInstruction& instr) {
    int arg_count = instr.arg;
    std::vector<std::shared_ptr<IRValue>> args;
    
    // Pop arguments in reverse order (Python stack order)
    for (int i = 0; i < arg_count; ++i) {
        args.insert(args.begin(), popStack());
    }
    
    auto callee = popStack();
    auto result_type = type_system.getAnyType();
    auto result = createTemporary(result_type);
    auto call = std::make_unique<IRCall>(result_type, callee, args);
    current_block->addInstruction(std::move(call));
    pushStack(result);
}

void BytecodeTranslator::translateBuildTuple(const BytecodeInstruction& instr) {
    int count = instr.arg;
    std::vector<std::shared_ptr<IRValue>> elements;
    
    for (int i = 0; i < count; ++i) {
        elements.insert(elements.begin(), popStack());
    }
    
    auto tuple_type = type_system.createTupleType(elements);
    auto result = createTemporary(tuple_type);
    auto build = std::make_unique<IRBuildTuple>(tuple_type, elements);
    current_block->addInstruction(std::move(build));
    pushStack(result);
}

void BytecodeTranslator::translateBuildList(const BytecodeInstruction& instr) {
    int count = instr.arg;
    std::vector<std::shared_ptr<IRValue>> elements;
    
    for (int i = 0; i < count; ++i) {
        elements.insert(elements.begin(), popStack());
    }
    
    auto list_type = type_system.createListType(type_system.getAnyType());
    auto result = createTemporary(list_type);
    auto build = std::make_unique<IRBuildList>(list_type, elements);
    current_block->addInstruction(std::move(build));
    pushStack(result);
}

void BytecodeTranslator::translateBuildSet(const BytecodeInstruction& instr) {
    int count = instr.arg;
    std::vector<std::shared_ptr<IRValue>> elements;
    
    for (int i = 0; i < count; ++i) {
        elements.insert(elements.begin(), popStack());
    }
    
    auto set_type = type_system.createSetType(type_system.getAnyType());
    auto result = createTemporary(set_type);
    auto build = std::make_unique<IRBuildSet>(set_type, elements);
    current_block->addInstruction(std::move(build));
    pushStack(result);
}

void BytecodeTranslator::translateBuildMap(const BytecodeInstruction& instr) {
    int count = instr.arg;
    std::vector<std::shared_ptr<IRValue>> keys, values;
    
    for (int i = 0; i < count; ++i) {
        auto value = popStack();
        auto key = popStack();
        keys.insert(keys.begin(), key);
        values.insert(values.begin(), value);
    }
    
    auto dict_type = type_system.createDictType(type_system.getAnyType(), type_system.getAnyType());
    auto result = createTemporary(dict_type);
    auto build = std::make_unique<IRBuildMap>(dict_type, keys, values);
    current_block->addInstruction(std::move(build));
    pushStack(result);
}

void BytecodeTranslator::translateReturnValue(const BytecodeInstruction& instr) {
    auto value = popStack();
    auto ret = std::make_unique<IRReturn>(value);
    current_block->addInstruction(std::move(ret));
}

// Helper methods implementation
std::shared_ptr<IRValue> BytecodeTranslator::popStack() {
    if (stack.empty()) {
        throw std::runtime_error("Stack underflow");
    }
    auto value = stack.back();
    stack.pop_back();
    return value;
}

void BytecodeTranslator::pushStack(std::shared_ptr<IRValue> value) {
    stack.push_back(value);
}

std::shared_ptr<IRValue> BytecodeTranslator::getConstant(int index) {
    auto it = const_map.find(index);
    if (it != const_map.end()) {
        return it->second;
    }
    
    // Create a new constant if not found
    auto const_type = type_system.getAnyType();
    auto constant = std::make_shared<IRConstant>(0, const_type, "const_" + std::to_string(index));
    const_map[index] = constant;
    return constant;
}

std::shared_ptr<IRValue> BytecodeTranslator::getLocal(int index) {
    auto it = local_map.find(index);
    if (it != local_map.end()) {
        return it->second;
    }
    
    auto local_type = type_system.getAnyType();
    auto local = std::make_shared<IRLocal>(local_type, "local_" + std::to_string(index));
    local_map[index] = local;
    return local;
}

std::shared_ptr<IRValue> BytecodeTranslator::getGlobal(int index) {
    auto it = global_map.find(index);
    if (it != global_map.end()) {
        return it->second;
    }
    
    auto global_type = type_system.getAnyType();
    auto global = std::make_shared<IRGlobal>(global_type, "global_" + std::to_string(index));
    global_map[index] = global;
    return global;
}

void BytecodeTranslator::setLocal(int index, std::shared_ptr<IRValue> value) {
    local_map[index] = value;
}

void BytecodeTranslator::setGlobal(int index, std::shared_ptr<IRValue> value) {
    global_map[index] = value;
}

std::shared_ptr<IRValue> BytecodeTranslator::createTemporary(std::shared_ptr<Type> type) {
    auto temp = std::make_shared<IRTemporary>(temp_counter++, type);
    return temp;
}

IRBasicBlock* BytecodeTranslator::getOrCreateBlock(uint32_t offset) {
    auto it = block_map.find(offset);
    if (it != block_map.end()) {
        return it->second.get();
    }
    
    auto block = std::make_unique<IRBasicBlock>("block_" + std::to_string(offset));
    auto block_ptr = block.get();
    block_map[offset] = std::move(block);
    return block_ptr;
}

void BytecodeTranslator::setCurrentBlock(IRBasicBlock* block) {
    current_block.reset(block);
}

void BytecodeTranslator::reset() {
    ir_module.reset();
    current_function.reset();
    current_block.reset();
    block_map.clear();
    const_map.clear();
    local_map.clear();
    global_map.clear();
    stack.clear();
    temp_counter = 0;
}

// Utility functions
bool BytecodeTranslator::isJumpInstruction(uint8_t opcode) {
    switch (static_cast<BytecodeOpcode>(opcode)) {
        case BytecodeOpcode::JUMP_FORWARD:
        case BytecodeOpcode::JUMP_ABSOLUTE:
        case BytecodeOpcode::POP_JUMP_IF_TRUE:
        case BytecodeOpcode::POP_JUMP_IF_FALSE:
        case BytecodeOpcode::JUMP_IF_TRUE_OR_POP:
        case BytecodeOpcode::JUMP_IF_FALSE_OR_POP:
            return true;
        default:
            return false;
    }
}

uint32_t BytecodeTranslator::getJumpTarget(const BytecodeInstruction& instr) {
    switch (static_cast<BytecodeOpcode>(instr.opcode)) {
        case BytecodeOpcode::JUMP_FORWARD:
            return instr.offset + 2 + instr.arg;
        case BytecodeOpcode::JUMP_ABSOLUTE:
        case BytecodeOpcode::POP_JUMP_IF_TRUE:
        case BytecodeOpcode::POP_JUMP_IF_FALSE:
            return instr.arg;
        case BytecodeOpcode::JUMP_IF_TRUE_OR_POP:
        case BytecodeOpcode::JUMP_IF_FALSE_OR_POP:
            return instr.arg;
        default:
            return instr.offset + 2;
    }
}

// AOTCompilationPipeline implementation
AOTCompilationPipeline::AOTCompilationPipeline() 
    : parser(std::make_unique<BytecodeParser>()),
      translator(std::make_unique<BytecodeTranslator>(type_system)),
      codegen(std::make_unique<LLVMCodeGenerator>(type_system)) {
}

bool AOTCompilationPipeline::compile(const std::string& input, const std::string& output) {
    input_file = input;
    output_file = output;
    
    if (verbose) {
        std::cout << "Starting AOT compilation of " << input << " -> " << output << std::endl;
    }
    
    // Stage 1: Parse input
    if (!parseInput(input)) {
        std::cerr << "Failed to parse input file: " << input << std::endl;
        return false;
    }
    
    // Stage 2: Translate to IR
    if (!translateToIR()) {
        std::cerr << "Failed to translate to IR" << std::endl;
        return false;
    }
    
    // Stage 3: Generate native code
    if (!generateNativeCode()) {
        std::cerr << "Failed to generate native code" << std::endl;
        return false;
    }
    
    // Stage 4: Optimize code
    if (optimization_level > 0 && !optimizeCode()) {
        std::cerr << "Failed to optimize code" << std::endl;
        return false;
    }
    
    // Stage 5: Link executable
    if (!linkExecutable()) {
        std::cerr << "Failed to link executable" << std::endl;
        return false;
    }
    
    if (verbose) {
        std::cout << "AOT compilation completed successfully!" << std::endl;
    }
    
    return true;
}

bool AOTCompilationPipeline::parseInput(const std::string& input) {
    // Try to parse as bytecode file first
    if (parser->parseBytecodeFile(input)) {
        return true;
    }
    
    // If that fails, we might need to compile Python source to bytecode first
    // For now, return false
    std::cerr << "Cannot parse input file: " << input << std::endl;
    return false;
}

bool AOTCompilationPipeline::translateToIR() {
    auto bytecode = parser->getInstructions();
    auto constants = parser->getConstants();
    
    translator->setConstants(constants);
    auto ir_module = translator->translate(bytecode);
    
    if (!ir_module) {
        return false;
    }
    
    return codegen->generate(*ir_module);
}

bool AOTCompilationPipeline::generateNativeCode() {
    return codegen->writeToObjectFile(output_file + ".o");
}

bool AOTCompilationPipeline::optimizeCode() {
    return codegen->optimize(optimization_level);
}

bool AOTCompilationPipeline::linkExecutable() {
    return codegen->generateNativeExecutable(output_file);
}

} // namespace pyplusplus