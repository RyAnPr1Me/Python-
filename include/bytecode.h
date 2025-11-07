#pragma once
#include "ast.h"
#include "ir.h"
#include "codegen.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

namespace pyplusplus {

// Python bytecode instruction structure
struct BytecodeInstruction {
    uint8_t opcode;
    uint16_t arg;
    uint32_t offset;
    std::string comment;
    
    BytecodeInstruction(uint8_t op, uint16_t a, uint32_t off, const std::string& c = "")
        : opcode(op), arg(a), offset(off), comment(c) {}
};

// Python bytecode types
enum class BytecodeOpcode {
    // Stack operations
    LOAD_CONST = 100, LOAD_FAST, LOAD_GLOBAL, LOAD_DEREF, LOAD_NAME,
    STORE_FAST, STORE_GLOBAL, STORE_NAME, STORE_DEREF,
    DELETE_FAST, DELETE_GLOBAL, DELETE_NAME, DELETE_DEREF,
    
    // Binary operations
    BINARY_POWER, BINARY_MULTIPLY, BINARY_MODULO, BINARY_ADD,
    BINARY_SUBTRACT, BINARY_FLOOR_DIVIDE, BINARY_TRUE_DIVIDE,
    INPLACE_ADD, INPLACE_SUBTRACT, INPLACE_MULTIPLY, INPLACE_MODULO,
    INPLACE_POWER, INPLACE_FLOOR_DIVIDE, INPLACE_TRUE_DIVIDE,
    
    // Unary operations
    UNARY_POSITIVE, UNARY_NEGATIVE, UNARY_NOT, UNARY_INVERT,
    
    // Comparison operations
    COMPARE_OP,
    
    // Jumps
    JUMP_FORWARD, JUMP_IF_TRUE_OR_POP, JUMP_IF_FALSE_OR_POP,
    JUMP_ABSOLUTE, POP_JUMP_IF_TRUE, POP_JUMP_IF_FALSE,
    
    // Function calls
    CALL_FUNCTION, CALL_FUNCTION_KW, CALL_FUNCTION_EX, CALL_METHOD,
    
    // Collections
    BUILD_TUPLE, BUILD_LIST, BUILD_SET, BUILD_MAP,
    BUILD_CONST_KEY_MAP, BUILD_SLICE,
    
    // Attributes and subscripts
    LOAD_ATTR, STORE_ATTR, DELETE_ATTR, LOAD_SUBSCR, STORE_SUBSCR, DELETE_SUBSCR,
    
    // Iteration
    GET_ITER, FOR_ITER, GET_AITER, GET_ANEXT, BEFORE_ASYNC_WITH,
    
    // Exception handling
    SETUP_FINALLY, SETUP_WITH, SETUP_ASYNC_WITH,
    POP_BLOCK, POP_EXCEPT, RERAISE, RAISE_VARARGS,
    
    // Import system
    IMPORT_NAME, IMPORT_FROM, IMPORT_STAR,
    
    // Control flow
    BREAK_LOOP, CONTINUE_LOOP, RETURN_VALUE, YIELD_VALUE, YIELD_FROM,
    
    // Other
    PRINT_EXPR, LOAD_BUILD_CLASS, MAKE_FUNCTION, SLICE_0, SLICE_1, SLICE_2, SLICE_3,
    STORE_SLICE_0, STORE_SLICE_1, STORE_SLICE_2, STORE_SLICE_3,
    DELETE_SLICE_0, DELETE_SLICE_1, DELETE_SLICE_2, DELETE_SLICE_3,
    ROT_TWO, ROT_THREE, ROT_FOUR, DUP_TOP, DUP_TOP_TWO,
    
    // Extended arguments
    EXTENDED_ARG
};

// Bytecode to IR translator
class BytecodeTranslator {
private:
    std::unique_ptr<IRModule> ir_module;
    std::unique_ptr<IRFunction> current_function;
    std::unique_ptr<IRBasicBlock> current_block;
    std::unordered_map<uint32_t, std::unique_ptr<IRBasicBlock>> block_map;
    std::unordered_map<int, std::shared_ptr<IRValue>> const_map;
    std::unordered_map<int, std::shared_ptr<IRValue>> local_map;
    std::unordered_map<int, std::shared_ptr<IRValue>> global_map;
    std::vector<std::shared_ptr<IRValue>> stack;
    TypeSystem& type_system;
    int temp_counter = 0;
    
    // Translation helpers
    std::shared_ptr<IRValue> popStack();
    void pushStack(std::shared_ptr<IRValue> value);
    std::shared_ptr<IRValue> getConstant(int index);
    std::shared_ptr<IRValue> getLocal(int index);
    std::shared_ptr<IRValue> getGlobal(int index);
    void setLocal(int index, std::shared_ptr<IRValue> value);
    void setGlobal(int index, std::shared_ptr<IRValue> value);
    std::shared_ptr<IRValue> createTemporary(std::shared_ptr<Type> type);
    
    // Block management
    IRBasicBlock* getOrCreateBlock(uint32_t offset);
    void setCurrentBlock(IRBasicBlock* block);
    
    // Instruction translators
    void translateLoadConst(const BytecodeInstruction& instr);
    void translateLoadFast(const BytecodeInstruction& instr);
    void translateStoreFast(const BytecodeInstruction& instr);
    void translateLoadGlobal(const BytecodeInstruction& instr);
    void translateStoreGlobal(const BytecodeInstruction& instr);
    void translateBinaryOp(const BytecodeInstruction& instr);
    void translateUnaryOp(const BytecodeInstruction& instr);
    void translateCompareOp(const BytecodeInstruction& instr);
    void translateJump(const BytecodeInstruction& instr);
    void translateJumpIf(const BytecodeInstruction& instr);
    void translateCallFunction(const BytecodeInstruction& instr);
    void translateBuildTuple(const BytecodeInstruction& instr);
    void translateBuildList(const BytecodeInstruction& instr);
    void translateBuildSet(const BytecodeInstruction& instr);
    void translateBuildMap(const BytecodeInstruction& instr);
    void translateLoadAttr(const BytecodeInstruction& instr);
    void translateStoreAttr(const BytecodeInstruction& instr);
    void translateLoadSubscr(const BytecodeInstruction& instr);
    void translateStoreSubscr(const BytecodeInstruction& instr);
    void translateReturnValue(const BytecodeInstruction& instr);
    void translateGetIter(const BytecodeInstruction& instr);
    void translateForIter(const BytecodeInstruction& instr);
    void translateImportName(const BytecodeInstruction& instr);
    void translateMakeFunction(const BytecodeInstruction& instr);
    
    // Advanced translation
    void translateListComprehension(const std::vector<BytecodeInstruction>& instructions);
    void translateGeneratorExpression(const std::vector<BytecodeInstruction>& instructions);
    void translateAsyncFunction(const std::vector<BytecodeInstruction>& instructions);
    void translateClassDefinition(const std::vector<BytecodeInstruction>& instructions);
    
public:
    explicit BytecodeTranslator(TypeSystem& type_system);
    
    // Main translation method
    std::unique_ptr<IRModule> translate(const std::vector<BytecodeInstruction>& bytecode, 
                                       const std::string& module_name = "__main__");
    
    // Utility methods
    void reset();
    void setConstants(const std::vector<std::shared_ptr<IRValue>>& constants);
    void setLocals(const std::vector<std::string>& local_names);
    void setGlobals(const std::vector<std::string>& global_names);
    
    // Debug and analysis
    std::string dumpIR() const;
    void analyzeBytecode(const std::vector<BytecodeInstruction>& bytecode);
};

// Python bytecode parser
class BytecodeParser {
private:
    std::vector<uint8_t> bytecode_data;
    std::vector<std::shared_ptr<IRValue>> constants;
    std::vector<std::string> names;
    std::vector<std::string> varnames;
    std::vector<std::string> freevars;
    std::vector<std::string> cellvars;
    
public:
    BytecodeParser();
    
    // Parse compiled Python code object
    bool parseCodeObject(const void* code_obj);
    bool parseBytecodeFile(const std::string& filename);
    
    // Get parsed data
    std::vector<BytecodeInstruction> getInstructions() const;
    const std::vector<std::shared_ptr<IRValue>>& getConstants() const { return constants; }
    const std::vector<std::string>& getNames() const { return names; }
    const std::vector<std::string>& getVarNames() const { return varnames; }
    
    // Utility methods
    void clear();
    std::string disassemble() const;
};

// AOT compilation pipeline
class AOTCompilationPipeline {
private:
    TypeSystem type_system;
    std::unique_ptr<BytecodeParser> parser;
    std::unique_ptr<BytecodeTranslator> translator;
    std::unique_ptr<LLVMCodeGenerator> codegen;
    
    // Pipeline stages
    bool parseInput(const std::string& input);
    bool translateToIR();
    bool generateNativeCode();
    bool optimizeCode();
    bool linkExecutable();
    
    // Configuration
    std::string input_file;
    std::string output_file;
    int optimization_level = 2;
    bool debug_mode = false;
    bool verbose = false;
    
public:
    AOTCompilationPipeline();
    
    // Main compilation method
    bool compile(const std::string& input, const std::string& output);
    
    // Configuration
    void setOptimizationLevel(int level) { optimization_level = level; }
    void setDebugMode(bool debug) { debug_mode = debug; }
    void setVerbose(bool verbose) { this->verbose = verbose; }
    
    // Advanced features
    bool enableProfileGuidedOptimization(const std::string& profile_file);
    bool enableLinkTimeOptimization();
    bool enableCrossCompilation(const std::string& target_triple);
    
    // Analysis and debugging
    void dumpIntermediateStages() const;
    void analyzePerformance() const;
};

// Utility functions
namespace bytecode_utils {
    // Convert Python opcode to string
    std::string opcodeToString(BytecodeOpcode opcode);
    
    // Analyze bytecode patterns
    bool isLoop(const std::vector<BytecodeInstruction>& bytecode, uint32_t start);
    bool isFunctionCall(const std::vector<BytecodeInstruction>& bytecode, uint32_t start);
    bool isExceptionHandling(const std::vector<BytecodeInstruction>& bytecode, uint32_t start);
    
    // Optimize bytecode
    std::vector<BytecodeInstruction> optimizeBytecode(const std::vector<BytecodeInstruction>& bytecode);
    std::vector<BytecodeInstruction> inlineFunctions(const std::vector<BytecodeInstruction>& bytecode);
    std::vector<BytecodeInstruction> unrollLoops(const std::vector<BytecodeInstruction>& bytecode);
    
    // Convert between different representations
    std::vector<BytecodeInstruction> astToBytecode(const Module& ast_module);
    Module bytecodeToAST(const std::vector<BytecodeInstruction>& bytecode);
}

} // namespace pyplusplus