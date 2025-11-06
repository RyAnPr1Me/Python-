#include "codegen.h"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/IR/Verifier.h>

namespace pyplusplus {

LLVMCodeGenerator::LLVMCodeGenerator(TypeSystem& type_system) 
    : type_system(type_system), context(std::make_unique<llvm::LLVMContext>()),
      builder(std::make_unique<llvm::IRBuilder<>>(*context)) {
    
    // Initialize LLVM
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    
    initializeTypeMap();
}

void LLVMCodeGenerator::initializeTypeMap() {
    // Map Python types to LLVM types
    type_map[type_system.getBoolType().get()] = llvm::Type::getInt1Ty(*context);
    type_map[type_system.getIntType().get()] = llvm::Type::getInt64Ty(*context);
    type_map[type_system.getFloatType().get()] = llvm::Type::getDoubleTy(*context);
    
    // String type - represented as pointer to char
    type_map[type_system.getStringType().get()] = llvm::Type::getInt8PtrTy(*context);
    
    // None type - represented as void pointer
    type_map[type_system.getNoneType().get()] = llvm::Type::getInt8PtrTy(*context);
    
    // Any type - represented as void pointer (Python object)
    type_map[type_system.getAnyType().get()] = llvm::Type::getInt8PtrTy(*context);
}

llvm::Type* LLVMCodeGenerator::getLLVMType(const Type* type) {
    auto it = type_map.find(type);
    if (it != type_map.end()) {
        return it->second;
    }
    
    // Handle complex types
    switch (type->getKind()) {
        case Type::Kind::LIST: {
            auto list_type = static_cast<const ListType*>(type);
            auto element_type = getLLVMType(list_type->getElementType());
            // List structure: { int64_t size, element_type* data }
            auto struct_type = llvm::StructType::create(*context, "list");
            struct_type->setBody({
                llvm::Type::getInt64Ty(*context),  // size
                llvm::PointerType::get(element_type, 0)  // data pointer
            });
            type_map[type] = struct_type;
            return struct_type;
        }
        
        case Type::Kind::DICT: {
            auto dict_type = static_cast<const DictType*>(type);
            // Dict structure: { int64_t size, void* data }
            auto struct_type = llvm::StructType::create(*context, "dict");
            struct_type->setBody({
                llvm::Type::getInt64Ty(*context),  // size
                llvm::Type::getInt8PtrTy(*context)  // data pointer
            });
            type_map[type] = struct_type;
            return struct_type;
        }
        
        case Type::Kind::FUNCTION: {
            auto func_type = static_cast<const FunctionType*>(type);
            std::vector<llvm::Type*> param_types;
            for (const auto& param_type : func_type->getParameterTypes()) {
                param_types.push_back(getLLVMType(param_type.get()));
            }
            auto return_type = getLLVMType(func_type->getReturnType());
            auto llvm_func_type = llvm::FunctionType::get(return_type, param_types, false);
            type_map[type] = llvm_func_type;
            return llvm_func_type;
        }
        
        case Type::Kind::CLASS: {
            // Class represented as void pointer (Python object)
            auto struct_type = llvm::StructType::create(*context, "class");
            struct_type->setBody({llvm::Type::getInt8PtrTy(*context)});
            type_map[type] = struct_type;
            return struct_type;
        }
        
        case Type::Kind::UNION: {
            // Union represented as the most general type (void pointer)
            type_map[type] = llvm::Type::getInt8PtrTy(*context);
            return llvm::Type::getInt8PtrTy(*context);
        }
        
        default:
            // Default to void pointer for unknown types
            type_map[type] = llvm::Type::getInt8PtrTy(*context);
            return llvm::Type::getInt8PtrTy(*context);
    }
}

llvm::Value* LLVMCodeGenerator::getLLVMValue(const IRValue* value) {
    auto it = value_map.find(value);
    if (it != value_map.end()) {
        return it->second;
    }
    
    // Generate constants
    if (auto constant = dynamic_cast<const IRConstant*>(value)) {
        llvm::Value* llvm_value = nullptr;
        
        if (constant->isInt()) {
            llvm_value = llvm::ConstantInt::get(getLLVMType(constant->getType()), constant->asInt());
        } else if (constant->isFloat()) {
            llvm_value = llvm::ConstantFP::get(getLLVMType(constant->getType()), constant->asFloat());
        } else if (constant->isBool()) {
            llvm_value = llvm::ConstantInt::get(getLLVMType(constant->getType()), constant->asBool());
        } else if (constant->isString()) {
            llvm_value = createStringLiteral(constant->asString());
        } else if (constant->isNone()) {
            llvm_value = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(getLLVMType(constant->getType())));
        }
        
        value_map[value] = llvm_value;
        return llvm_value;
    }
    
    // Handle parameters, locals, globals, temporaries
    // These should be generated during function generation
    return nullptr;
}

llvm::BasicBlock* LLVMCodeGenerator::getLLVMBasicBlock(const IRBasicBlock* block) {
    auto it = block_map.find(block);
    if (it != block_map.end()) {
        return it->second;
    }
    
    // Create new basic block
    auto llvm_block = llvm::BasicBlock::Create(*context, block->getName());
    block_map[block] = llvm_block;
    return llvm_block;
}

llvm::Function* LLVMCodeGenerator::getLLVMFunction(const IRFunction* function) {
    auto it = function_map.find(function);
    if (it != function_map.end()) {
        return it->second;
    }
    
    // Create new function
    auto func_type = static_cast<llvm::FunctionType*>(getLLVMType(function->getType()));
    auto llvm_func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, 
                                           function->getName(), module.get());
    
    // Set parameter names
    size_t i = 0;
    for (auto& arg : llvm_func->args()) {
        if (i < function->getParameters().size()) {
            arg.setName(function->getParameters()[i]->getName());
        }
        i++;
    }
    
    function_map[function] = llvm_func;
    return llvm_func;
}

bool LLVMCodeGenerator::generate(const IRModule& ir_module) {
    // Create LLVM module
    module = std::make_unique<llvm::Module>(ir_module.getName(), *context);
    module->setDataLayout("");  // Will be set by target machine
    
    // Generate all functions
    for (const auto& function : ir_module.getFunctions()) {
        generateFunction(function.get());
    }
    
    // Verify module
    std::string error;
    llvm::raw_string_ostream error_stream(error);
    if (llvm::verifyModule(*module, &error_stream)) {
        std::cerr << "Module verification failed: " << error << std::endl;
        return false;
    }
    
    return true;
}

void LLVMCodeGenerator::generateFunction(const IRFunction* ir_function) {
    auto llvm_func = getLLVMFunction(ir_function);
    
    // Create entry block
    auto entry_block = llvm::BasicBlock::Create(*context, "entry", llvm_func);
    builder->SetInsertPoint(entry_block);
    
    // Allocate space for parameters and locals
    std::unordered_map<std::string, llvm::Value*> locals;
    
    // Handle parameters
    size_t param_index = 0;
    for (auto& arg : llvm_func->args()) {
        if (param_index < ir_function->getParameters().size()) {
            auto param = ir_function->getParameters()[param_index];
            auto alloca = builder->CreateAlloca(getLLVMType(param->getType()), nullptr, param->getName() + "_addr");
            builder->CreateStore(&arg, alloca);
            locals[param->getName()] = alloca;
            value_map[param.get()] = &arg;
        }
        param_index++;
    }
    
    // Generate basic blocks
    for (const auto& block : ir_function->getBasicBlocks()) {
        auto llvm_block = getLLVMBasicBlock(block.get());
        if (block != ir_function->getBasicBlocks()[0].get()) {
            llvm_func->getBasicBlockList().push_back(llvm_block);
        }
    }
    
    // Generate instructions for each block
    for (const auto& block : ir_function->getBasicBlocks()) {
        auto llvm_block = getLLVMBasicBlock(block.get());
        builder->SetInsertPoint(llvm_block);
        
        for (const auto& instruction : block->getInstructions()) {
            generateInstruction(instruction.get());
        }
    }
}

void LLVMCodeGenerator::generateBasicBlock(const IRBasicBlock* ir_block) {
    auto llvm_block = getLLVMBasicBlock(ir_block);
    builder->SetInsertPoint(llvm_block);
    
    for (const auto& instruction : ir_block->getInstructions()) {
        generateInstruction(instruction.get());
    }
}

void LLVMCodeGenerator::generateInstruction(const IRInstruction* instruction) {
    llvm::Value* result = nullptr;
    
    switch (instruction->getOp()) {
        case IROp::ADD:
        case IROp::SUB:
        case IROp::MUL:
        case IROp::DIV:
        case IROp::MOD:
        case IROp::POW:
        case IROp::EQ:
        case IROp::NE:
        case IROp::LT:
        case IROp::LE:
        case IROp::GT:
        case IROp::GE:
        case IROp::AND:
        case IROp::OR:
        case IROp::BIT_AND:
        case IROp::BIT_OR:
        case IROp::BIT_XOR:
        case IROp::LEFT_SHIFT:
        case IROp::RIGHT_SHIFT:
            result = generateBinaryOp(static_cast<const IRBinaryOp*>(instruction));
            break;
            
        case IROp::NOT:
        case IROp::BIT_NOT:
            result = generateUnaryOp(static_cast<const IRUnaryOp*>(instruction));
            break;
            
        case IROp::CALL:
            result = generateCall(static_cast<const IRCall*>(instruction));
            break;
            
        case IROp::LOAD:
            result = generateLoad(static_cast<const IRLoad*>(instruction));
            break;
            
        case IROp::STORE:
            result = generateStore(static_cast<const IRStore*>(instruction));
            break;
            
        case IROp::ALLOC:
            result = generateAlloc(static_cast<const IRAlloc*>(instruction));
            break;
            
        case IROp::RETURN:
            result = generateReturn(static_cast<const IRReturn*>(instruction));
            break;
            
        case IROp::JUMP:
            result = generateJump(static_cast<const IRJump*>(instruction));
            break;
            
        case IROp::JUMP_IF_TRUE:
        case IROp::JUMP_IF_FALSE:
            result = generateJumpIf(static_cast<const IRJumpIf*>(instruction));
            break;
            
        default:
            std::cerr << "Unsupported IR operation: " << static_cast<int>(instruction->getOp()) << std::endl;
            break;
    }
    
    // Store result for temporary values
    if (result && instruction->getResultType()) {
        // Create a temporary value mapping
        // This is a simplified approach - in practice, we'd need better value tracking
    }
}

llvm::Value* LLVMCodeGenerator::generateBinaryOp(const IRBinaryOp* instruction) {
    auto left = getLLVMValue(instruction->getOperands()[0].get());
    auto right = getLLVMValue(instruction->getOperands()[1].get());
    
    if (!left || !right) {
        return nullptr;
    }
    
    switch (instruction->getOp()) {
        case IROp::ADD:
            return builder->CreateAdd(left, right);
        case IROp::SUB:
            return builder->CreateSub(left, right);
        case IROp::MUL:
            return builder->CreateMul(left, right);
        case IROp::DIV:
            return builder->CreateSDiv(left, right);
        case IROp::MOD:
            return builder->CreateSRem(left, right);
        case IROp::EQ:
            return builder->CreateICmpEQ(left, right);
        case IROp::NE:
            return builder->CreateICmpNE(left, right);
        case IROp::LT:
            return builder->CreateICmpSLT(left, right);
        case IROp::LE:
            return builder->CreateICmpSLE(left, right);
        case IROp::GT:
            return builder->CreateICmpSGT(left, right);
        case IROp::GE:
            return builder->CreateICmpSGE(left, right);
        case IROp::AND:
            return builder->CreateAnd(left, right);
        case IROp::OR:
            return builder->CreateOr(left, right);
        case IROp::BIT_AND:
            return builder->CreateAnd(left, right);
        case IROp::BIT_OR:
            return builder->CreateOr(left, right);
        case IROp::BIT_XOR:
            return builder->CreateXor(left, right);
        case IROp::LEFT_SHIFT:
            return builder->CreateShl(left, right);
        case IROp::RIGHT_SHIFT:
            return builder->CreateLShr(left, right);
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGenerator::generateUnaryOp(const IRUnaryOp* instruction) {
    auto operand = getLLVMValue(instruction->getOperands()[0].get());
    
    if (!operand) {
        return nullptr;
    }
    
    switch (instruction->getOp()) {
        case IROp::NOT:
            return builder->CreateNot(operand);
        case IROp::BIT_NOT:
            return builder->CreateNot(operand);
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGenerator::generateCall(const IRCall* instruction) {
    auto callee = getLLVMValue(instruction->getCallee());
    if (!callee) {
        return nullptr;
    }
    
    std::vector<llvm::Value*> args;
    for (const auto& arg : instruction->getArguments()) {
        auto llvm_arg = getLLVMValue(arg.get());
        if (!llvm_arg) {
            return nullptr;
        }
        args.push_back(llvm_arg);
    }
    
    return builder->CreateCall(callee, args);
}

llvm::Value* LLVMCodeGenerator::generateLoad(const IRLoad* instruction) {
    auto address = getLLVMValue(instruction->getAddress());
    if (!address) {
        return nullptr;
    }
    
    return builder->CreateLoad(address);
}

llvm::Value* LLVMCodeGenerator::generateStore(const IRStore* instruction) {
    auto value = getLLVMValue(instruction->getValue());
    auto address = getLLVMValue(instruction->getAddress());
    
    if (!value || !address) {
        return nullptr;
    }
    
    return builder->CreateStore(value, address);
}

llvm::Value* LLVMCodeGenerator::generateAlloc(const IRAlloc* instruction) {
    auto type = getLLVMType(instruction->getResultType());
    return builder->CreateAlloca(type);
}

llvm::Value* LLVMCodeGenerator::generateReturn(const IRReturn* instruction) {
    if (instruction->getValue()) {
        auto value = getLLVMValue(instruction->getValue());
        if (value) {
            return builder->CreateRet(value);
        }
    }
    
    return builder->CreateRetVoid();
}

llvm::Value* LLVMCodeGenerator::generateJump(const IRJump* instruction) {
    auto target = getLLVMBasicBlock(instruction->getTarget());
    if (target) {
        return builder->CreateBr(target);
    }
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::generateJumpIf(const IRJumpIf* instruction) {
    auto condition = getLLVMValue(instruction->getCondition());
    auto true_target = getLLVMBasicBlock(instruction->getTrueTarget());
    auto false_target = getLLVMBasicBlock(instruction->getFalseTarget());
    
    if (condition && true_target && false_target) {
        return builder->CreateCondBr(condition, true_target, false_target);
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGenerator::createStringLiteral(const std::string& str) {
    auto constant = builder->CreateGlobalStringPtr(str);
    return constant;
}

llvm::Function* LLVMCodeGenerator::getOrCreateRuntimeFunction(const std::string& name, 
                                                               llvm::Type* return_type, 
                                                               std::vector<llvm::Type*> param_types) {
    auto func_type = llvm::FunctionType::get(return_type, param_types, false);
    auto func = module->getFunction(name);
    
    if (!func) {
        func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, module.get());
    }
    
    return func;
}

bool LLVMCodeGenerator::writeToFile(const std::string& filename) {
    std::error_code error;
    llvm::raw_fd_ostream file(filename, error, llvm::sys::fs::OF_None);
    
    if (error) {
        std::cerr << "Error opening file: " << error.message() << std::endl;
        return false;
    }
    
    module->print(file, nullptr);
    return true;
}

bool LLVMCodeGenerator::writeToObjectFile(const std::string& filename) {
    // Get target triple
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(target_triple);
    
    // Get target
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    
    if (!target) {
        std::cerr << "Failed to lookup target: " << error << std::endl;
        return false;
    }
    
    // Create target machine
    auto cpu = "generic";
    auto features = "";
    llvm::TargetOptions opt;
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto target_machine = target->createTargetMachine(target_triple, cpu, features, opt, rm);
    
    module->setDataLayout(target_machine->createDataLayout());
    
    // Write object file
    std::error_code err;
    llvm::raw_fd_ostream dest(filename, err, llvm::sys::fs::OF_None);
    
    if (err) {
        std::cerr << "Could not open file: " << err.message() << std::endl;
        return false;
    }
    
    llvm::legacy::PassManager pass;
    auto file_type = llvm::CGFT_ObjectFile;
    
    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        std::cerr << "TargetMachine can't emit a file of this type" << std::endl;
        return false;
    }
    
    pass.run(*module);
    dest.flush();
    
    return true;
}

bool LLVMCodeGenerator::optimize(int optimization_level) {
    if (!module) {
        return false;
    }
    
    llvm::legacy::PassManager pass_manager;
    
    if (optimization_level >= 1) {
        pass_manager.add(llvm::createInstructionCombiningPass());
        pass_manager.add(llvm::createReassociatePass());
        pass_manager.add(llvm::createGVNPass());
        pass_manager.add(llvm::createCFGSimplificationPass());
    }
    
    if (optimization_level >= 2) {
        pass_manager.add(llvm::createPromoteMemoryToRegisterPass());
        pass_manager.add(llvm::createInstructionCombiningPass());
        pass_manager.add(llvm::createCFGSimplificationPass());
    }
    
    if (optimization_level >= 3) {
        pass_manager.add(llvm::createAggressiveDCEPass());
        pass_manager.add(llvm::createCFGSimplificationPass());
    }
    
    pass_manager.run(*module);
    return true;
}

// IROptimizer implementation
IROptimizer::IROptimizer(LLVMCodeGenerator& code_gen) : code_gen(code_gen) {}

bool IROptimizer::optimize(IRModule& module) {
    // Perform IR-level optimizations
    performConstantFolding(module);
    performDeadCodeElimination(module);
    performCommonSubexpressionElimination(module);
    performLoopOptimizations(module);
    
    return true;
}

bool IROptimizer::optimizeLLVMModule(llvm::Module* llvm_module, int level) {
    llvm::legacy::PassManager pass_manager;
    
    if (level >= 1) {
        pass_manager.add(llvm::createInstructionCombiningPass());
        pass_manager.add(llvm::createReassociatePass());
        pass_manager.add(llvm::createGVNPass());
        pass_manager.add(llvm::createCFGSimplificationPass());
    }
    
    if (level >= 2) {
        pass_manager.add(llvm::createPromoteMemoryToRegisterPass());
        pass_manager.add(llvm::createInstructionCombiningPass());
        pass_manager.add(llvm::createCFGSimplificationPass());
    }
    
    if (level >= 3) {
        pass_manager.add(llvm::createAggressiveDCEPass());
        pass_manager.add(llvm::createCFGSimplificationPass());
        pass_manager.add(llvm::createLoopUnrollPass());
    }
    
    pass_manager.run(*llvm_module);
    return true;
}

void IROptimizer::performConstantFolding(IRModule& module) {
    // TODO: Implement constant folding at IR level
}

void IROptimizer::performDeadCodeElimination(IRModule& module) {
    // TODO: Implement dead code elimination at IR level
}

void IROptimizer::performCommonSubexpressionElimination(IRModule& module) {
    // TODO: Implement CSE at IR level
}

void IROptimizer::performLoopOptimizations(IRModule& module) {
    // TODO: Implement loop optimizations at IR level
}

} // namespace pyplusplus