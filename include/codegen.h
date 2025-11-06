#pragma once
#include "ir.h"
#include "types.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <memory>
#include <unordered_map>

namespace pyplusplus {

class LLVMCodeGenerator {
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    TypeSystem& type_system;
    
    // Type mapping
    std::unordered_map<const Type*, llvm::Type*> type_map;
    std::unordered_map<const IRValue*, llvm::Value*> value_map;
    std::unordered_map<const IRBasicBlock*, llvm::BasicBlock*> block_map;
    std::unordered_map<const IRFunction*, llvm::Function*> function_map;
    
    // Helper methods
    llvm::Type* getLLVMType(const Type* type);
    llvm::Value* getLLVMValue(const IRValue* value);
    llvm::BasicBlock* getLLVMBasicBlock(const IRBasicBlock* block);
    llvm::Function* getLLVMFunction(const IRFunction* function);
    
    // Instruction generation
    llvm::Value* generateBinaryOp(const IRBinaryOp* instruction);
    llvm::Value* generateUnaryOp(const IRUnaryOp* instruction);
    llvm::Value* generateCall(const IRCall* instruction);
    llvm::Value* generateLoad(const IRLoad* instruction);
    llvm::Value* generateStore(const IRStore* instruction);
    llvm::Value* generateAlloc(const IRAlloc* instruction);
    llvm::Value* generateReturn(const IRReturn* instruction);
    llvm::Value* generateJump(const IRJump* instruction);
    llvm::Value* generateJumpIf(const IRJumpIf* instruction);
    
    // Runtime functions
    llvm::Function* getOrCreateRuntimeFunction(const std::string& name, llvm::Type* return_type, 
                                               std::vector<llvm::Type*> param_types);
    
public:
    explicit LLVMCodeGenerator(TypeSystem& type_system);
    
    bool generate(const IRModule& ir_module);
    bool writeToFile(const std::string& filename);
    bool writeToObjectFile(const std::string& filename);
    bool optimize(int optimization_level = 2);
    
    llvm::Module* getLLVMModule() const { return module.get(); }
    
private:
    void initializeTypeMap();
    void generateFunction(const IRFunction* ir_function);
    void generateBasicBlock(const IRBasicBlock* ir_block);
    void generateInstruction(const IRInstruction* instruction);
    
    // Python runtime helpers
    llvm::Value* createPythonObject(llvm::Value* value, const Type* type);
    llvm::Value* extractPythonValue(llvm::Value* obj, const Type* type);
    llvm::Value* createStringLiteral(const std::string& str);
    llvm::Value* createListObject(llvm::Value* elements, const Type* element_type);
    llvm::Value* createDictObject(llvm::Value* keys, llvm::Value* values, 
                                   const Type* key_type, const Type* value_type);
};

// Optimization passes
class IROptimizer {
private:
    LLVMCodeGenerator& code_gen;
    
public:
    explicit IROptimizer(LLVMCodeGenerator& code_gen) : code_gen(code_gen) {}
    
    bool optimize(IRModule& module);
    bool optimizeLLVMModule(llvm::Module* llvm_module, int level);
    
private:
    void performConstantFolding(IRModule& module);
    void performDeadCodeElimination(IRModule& module);
    void performCommonSubexpressionElimination(IRModule& module);
    void performLoopOptimizations(IRModule& module);
};

} // namespace pyplusplus