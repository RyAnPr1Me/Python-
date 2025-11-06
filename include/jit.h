#pragma once
#include "ir.h"
#include "runtime.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>

namespace pyplusplus {

// Forward declarations
class JITCompiler;
class CodeCache;
class HotSpotProfiler;

// JIT compilation modes
enum class JITMode {
    INTERPRETED,    // Pure interpretation
    MIXED,         // Mix of interpretation and compilation
    COMPILED       // Full compilation
};

// Optimization levels
enum class OptimizationLevel {
    O0,  // No optimization
    O1,  // Basic optimizations
    O2,  // Standard optimizations
    O3   // Aggressive optimizations
};

// JIT compilation statistics
struct JITStats {
    size_t functions_compiled = 0;
    size_t compilation_time_ms = 0;
    size_t execution_time_ms = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    double speedup_factor = 1.0;
};

// Hot function information
struct HotFunctionInfo {
    std::string function_name;
    uint64_t call_count = 0;
    uint64_t execution_time_ns = 0;
    bool is_compiled = false;
    bool is_hot = false;
    OptimizationLevel opt_level = OptimizationLevel::O0;
    std::vector<uint8_t> compiled_code;
    void* compiled_function = nullptr;
};

// JIT compilation context
class JITContext {
private:
    JITMode mode;
    OptimizationLevel opt_level;
    size_t compilation_threshold;
    bool enable_profiling;
    bool enable_caching;
    
public:
    JITContext(JITMode mode = JITMode::MIXED,
               OptimizationLevel opt_level = OptimizationLevel::O2,
               size_t compilation_threshold = 1000,
               bool enable_profiling = true,
               bool enable_caching = true)
        : mode(mode), opt_level(opt_level), compilation_threshold(compilation_threshold),
          enable_profiling(enable_profiling), enable_caching(enable_caching) {}
    
    JITMode getMode() const { return mode; }
    OptimizationLevel getOptimizationLevel() const { return opt_level; }
    size_t getCompilationThreshold() const { return compilation_threshold; }
    bool isProfilingEnabled() const { return enable_profiling; }
    bool isCachingEnabled() const { return enable_caching; }
    
    void setMode(JITMode new_mode) { mode = new_mode; }
    void setOptimizationLevel(OptimizationLevel level) { opt_level = level; }
    void setCompilationThreshold(size_t threshold) { compilation_threshold = threshold; }
};

// Code cache for compiled functions
class CodeCache {
private:
    std::unordered_map<std::string, HotFunctionInfo> cache;
    std::mutex cache_mutex;
    size_t max_cache_size;
    size_t current_cache_size;
    
public:
    explicit CodeCache(size_t max_size = 1024 * 1024 * 100) // 100MB default
        : max_cache_size(max_size), current_cache_size(0) {}
    
    bool has(const std::string& function_name) const;
    HotFunctionInfo* get(const std::string& function_name);
    void put(const std::string& function_name, const HotFunctionInfo& info);
    void remove(const std::string& function_name);
    void clear();
    
    size_t size() const { return cache.size(); }
    size_t getCacheSize() const { return current_cache_size; }
    size_t getMaxCacheSize() const { return max_cache_size; }
    
    // Cache statistics
    std::vector<std::string> getHotFunctions() const;
    JITStats getStats() const;
    
private:
    void evictLRU();
    bool shouldEvict() const;
};

// Hot spot profiler
class HotSpotProfiler {
private:
    std::unordered_map<std::string, HotFunctionInfo> profile_data;
    std::mutex profile_mutex;
    bool enabled;
    
public:
    explicit HotSpotProfiler(bool enabled = true) : enabled(enabled) {}
    
    void recordFunctionCall(const std::string& function_name, uint64_t execution_time_ns);
    void recordFunctionCompilation(const std::string& function_name, void* compiled_function);
    
    std::vector<std::string> getHotFunctions(size_t threshold = 1000) const;
    HotFunctionInfo* getFunctionInfo(const std::string& function_name);
    
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool isEnabled() const { return enabled; }
    
    void reset();
    JITStats getStats() const;
    
private:
    bool isHot(const HotFunctionInfo& info, size_t threshold) const;
};

// JIT compiler interface
class JITCompiler {
private:
    std::unique_ptr<CodeCache> code_cache;
    std::unique_ptr<HotSpotProfiler> profiler;
    JITContext context;
    JITStats stats;
    std::mutex compiler_mutex;
    
    // LLVM JIT components (simplified)
    void* llvm_context;
    void* llvm_module;
    void* llvm_execution_engine;
    
public:
    explicit JITCompiler(const JITContext& context = JITContext());
    ~JITCompiler();
    
    // Main compilation interface
    void* compileFunction(const std::string& function_name, 
                          const std::vector<IRInstruction>& ir_code);
    
    // Execution interface
    Value executeFunction(const std::string& function_name, 
                         const std::vector<Value>& args);
    
    // Profiling interface
    void profileFunctionCall(const std::string& function_name, 
                            uint64_t execution_time_ns);
    
    // Cache management
    bool isFunctionCompiled(const std::string& function_name) const;
    void* getCompiledFunction(const std::string& function_name);
    void invalidateFunction(const std::string& function_name);
    void clearCache();
    
    // Configuration
    void setContext(const JITContext& new_context);
    const JITContext& getContext() const { return context; }
    
    // Statistics
    JITStats getStats() const;
    void resetStats();
    
    // Optimization passes
    void optimizeIR(std::vector<IRInstruction>& ir_code);
    void applyOptimizations(HotFunctionInfo& info);
    
private:
    // LLVM-specific compilation
    void initializeLLVM();
    void shutdownLLVM();
    void* compileIRToNative(const std::vector<IRInstruction>& ir_code);
    
    // Optimization passes
    void optimizeConstants(std::vector<IRInstruction>& ir_code);
    void optimizeDeadCode(std::vector<IRInstruction>& ir_code);
    void optimizeCommonSubexpressions(std::vector<IRInstruction>& ir_code);
    void optimizeLoopInvariants(std::vector<IRInstruction>& ir_code);
    void optimizeTailCalls(std::vector<IRInstruction>& ir_code);
    void optimizeInlining(std::vector<IRInstruction>& ir_code);
    
    // Code generation
    std::vector<uint8_t> generateNativeCode(const std::vector<IRInstruction>& ir_code);
    void* createExecutableMemory(const std::vector<uint8_t>& code);
    
    // Utility functions
    bool shouldCompile(const std::string& function_name) const;
    OptimizationLevel determineOptimizationLevel(const HotFunctionInfo& info) const;
    void updateStats(const std::string& function_name, uint64_t compilation_time_ms);
};

// JIT execution engine
class JITExecutionEngine {
private:
    std::unique_ptr<JITCompiler> compiler;
    Runtime* runtime;
    bool jit_enabled;
    
public:
    explicit JITExecutionEngine(Runtime* runtime, bool jit_enabled = true);
    ~JITExecutionEngine();
    
    // Function execution with JIT
    Value executeFunction(const std::string& function_name, 
                         const std::vector<Value>& args,
                         const std::vector<IRInstruction>& ir_code = {});
    
    // Module execution with JIT
    Value executeModule(const std::string& module_name,
                       const std::vector<IRInstruction>& ir_code);
    
    // Configuration
    void enableJIT(bool enabled = true) { jit_enabled = enabled; }
    bool isJITEnabled() const { return jit_enabled; }
    
    void setJITContext(const JITContext& context);
    JITCompiler* getCompiler() { return compiler.get(); }
    
    // Statistics
    JITStats getJITStats() const;
    void resetJITStats();
    
private:
    Value executeInterpreted(const std::vector<IRInstruction>& ir_code,
                            const std::vector<Value>& args);
    Value executeCompiled(void* compiled_function, const std::vector<Value>& args);
    
    uint64_t measureExecutionTime(std::function<void()> func);
};

// JIT optimization passes
class JITO Pass {
public:
    virtual ~JITPass() = default;
    virtual void apply(std::vector<IRInstruction>& ir_code) = 0;
    virtual const char* getName() const = 0;
};

class ConstantFoldingPass : public JITPass {
public:
    void apply(std::vector<IRInstruction>& ir_code) override;
    const char* getName() const override { return "Constant Folding"; }
};

class DeadCodeEliminationPass : public JITPass {
public:
    void apply(std::vector<IRInstruction>& ir_code) override;
    const char* getName() const override { return "Dead Code Elimination"; }
};

class CommonSubexpressionEliminationPass : public JITPass {
public:
    void apply(std::vector<IRInstruction>& ir_code) override;
    const char* getName() const override { return "Common Subexpression Elimination"; }
};

class LoopInvariantCodeMotionPass : public JITPass {
public:
    void apply(std::vector<IRInstruction>& ir_code) override;
    const char* getName() const override { return "Loop Invariant Code Motion"; }
};

class InliningPass : public JITPass {
private:
    size_t max_inline_size;
    
public:
    explicit InliningPass(size_t max_inline_size = 50) : max_inline_size(max_inline_size) {}
    void apply(std::vector<IRInstruction>& ir_code) override;
    const char* getName() const override { return "Function Inlining"; }
};

class TailCallOptimizationPass : public JITPass {
public:
    void apply(std::vector<IRInstruction>& ir_code) override;
    const char* getName() const override { return "Tail Call Optimization"; }
};

// JIT manager for coordinating all JIT components
class JITManager {
private:
    std::unique_ptr<JITExecutionEngine> execution_engine;
    std::vector<std::unique_ptr<JITPass>> optimization_passes;
    bool initialized;
    
public:
    explicit JITManager(Runtime* runtime);
    ~JITManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Execution interface
    Value executeFunction(const std::string& function_name,
                         const std::vector<Value>& args,
                         const std::vector<IRInstruction>& ir_code = {});
    
    // Configuration
    void setJITContext(const JITContext& context);
    void addOptimizationPass(std::unique_ptr<JITPass> pass);
    
    // Statistics and debugging
    JITStats getStats() const;
    void resetStats();
    void printStats() const;
    
    // Cache management
    void clearJITCache();
    void invalidateFunction(const std::string& function_name);
    
    bool isInitialized() const { return initialized; }
    JITExecutionEngine* getExecutionEngine() { return execution_engine.get(); }
    
private:
    void setupDefaultOptimizationPasses();
};

} // namespace pyplusplus