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
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace pyplusplus {

class LLVMCodeGenerator {
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    TypeSystem& type_system;
    std::unique_ptr<llvm::TargetMachine> target_machine;
    
    // Type mapping
    std::unordered_map<const Type*, llvm::Type*> type_map;
    std::unordered_map<const IRValue*, llvm::Value*> value_map;
    std::unordered_map<const IRBasicBlock*, llvm::BasicBlock*> block_map;
    std::unordered_map<const IRFunction*, llvm::Function*> function_map;
    
    // Python object representation
    struct PyObjectLayout {
        llvm::StructType* type;
        llvm::Value* ref_count;
        llvm::Value* ob_type;
        llvm::Value* ob_data;
    };
    
    PyObjectLayout py_object_layout;
    
    // Helper methods
    llvm::Type* getLLVMType(const Type* type);
    llvm::Value* getLLVMValue(const IRValue* value);
    llvm::BasicBlock* getLLVMBasicBlock(const IRBasicBlock* block);
    llvm::Function* getLLVMFunction(const IRFunction* function);
    
    // Enhanced instruction generation for Python semantics
    llvm::Value* generateBinaryOp(const IRBinaryOp* instruction);
    llvm::Value* generateUnaryOp(const IRUnaryOp* instruction);
    llvm::Value* generateCall(const IRCall* instruction);
    llvm::Value* generateLoad(const IRLoad* instruction);
    llvm::Value* generateStore(const IRStore* instruction);
    llvm::Value* generateAlloc(const IRAlloc* instruction);
    llvm::Value* generateReturn(const IRReturn* instruction);
    llvm::Value* generateJump(const IRJump* instruction);
    llvm::Value* generateJumpIf(const IRJumpIf* instruction);
    
    // Python-specific code generation
    llvm::Value* generatePythonBinaryOp(llvm::Value* left, llvm::Value* right, const std::string& op_name);
    llvm::Value* generatePythonUnaryOp(llvm::Value* operand, const std::string& op_name);
    llvm::Value* generatePythonCall(llvm::Value* callee, const std::vector<llvm::Value*>& args);
    llvm::Value* generatePythonAttributeAccess(llvm::Value* obj, const std::string& attr);
    llvm::Value* generatePythonSubscript(llvm::Value* obj, llvm::Value* index);
    llvm::Value* generatePythonSlice(llvm::Value* obj, llvm::Value* start, llvm::Value* end, llvm::Value* step);
    
    // Object creation and manipulation
    llvm::Value* createPythonObject(llvm::Type* py_type);
    llvm::Value* createPythonInt(int64_t value);
    llvm::Value* createPythonFloat(double value);
    llvm::Value* createPythonString(const std::string& str);
    llvm::Value* createPythonList(const std::vector<llvm::Value*>& elements);
    llvm::Value* createPythonDict(const std::vector<llvm::Value*>& keys, const std::vector<llvm::Value*>& values);
    llvm::Value* createPythonTuple(const std::vector<llvm::Value*>& elements);
    llvm::Value* createPythonSet(const std::vector<llvm::Value*>& elements);
    
    // Reference counting
    llvm::Value* incref(llvm::Value* obj);
    llvm::Value* decref(llvm::Value* obj);
    void generateRefCounting(llvm::Value* obj, bool increment = true);
    
    // Exception handling
    llvm::Value* generateExceptionCheck();
    llvm::Value* generateRaiseException(llvm::Value* exception);
    llvm::BasicBlock* createExceptionLandingPad();
    
    // Runtime functions
    llvm::Function* getOrCreateRuntimeFunction(const std::string& name, llvm::Type* return_type, 
                                               std::vector<llvm::Type*> param_types);
    
    // Target-specific optimizations
    void setupTargetMachine();
    void optimizeForTarget(llvm::Function* func);
    void initializePyObjectLayout();
    void declareRuntimeFunctions();
    void generateModuleInit(const IRModule& ir_module);
    void generateGlobalVariable(const IRGlobal* global);
    
public:
    explicit LLVMCodeGenerator(TypeSystem& type_system);
    
    bool generate(const IRModule& ir_module);
    bool writeToFile(const std::string& filename);
    bool writeToObjectFile(const std::string& filename);
    bool optimize(int optimization_level = 2);
    
    // Enhanced AOT compilation features
    bool generateNativeExecutable(const std::string& output_path);
    bool generateSharedLibrary(const std::string& output_path);
    bool generateHeaderFile(const std::string& output_path);
    
    // Multi-target support
    bool setTargetTriple(const std::string& triple);
    bool setTargetCPU(const std::string& cpu);
    bool setTargetFeatures(const std::string& features);
    
    // Advanced optimizations
    bool enableVectorization();
    bool enableInterproceduralOptimization();
    bool enableProfileGuidedOptimization(const std::string& profile_data);
    
    llvm::Module* getLLVMModule() const { return module.get(); }
    llvm::TargetMachine* getTargetMachine() const { return target_machine.get(); }
    
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