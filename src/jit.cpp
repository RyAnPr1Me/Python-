#include "jit.h"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <cstring>

#ifdef __APPLE__
#include <sys/mman.h>
#include <unistd.h>
#elif defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Unsupported platform"
#endif

namespace pyplusplus {

// CodeCache implementation
bool CodeCache::has(const std::string& function_name) const {
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache.find(function_name) != cache.end();
}

HotFunctionInfo* CodeCache::get(const std::string& function_name) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache.find(function_name);
    return it != cache.end() ? &it->second : nullptr;
}

void CodeCache::put(const std::string& function_name, const HotFunctionInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    // Check if we need to evict
    if (shouldEvict()) {
        evictLRU();
    }
    
    cache[function_name] = info;
    current_cache_size += info.compiled_code.size();
}

void CodeCache::remove(const std::string& function_name) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache.find(function_name);
    if (it != cache.end()) {
        current_cache_size -= it->second.compiled_code.size();
        cache.erase(it);
    }
}

void CodeCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache.clear();
    current_cache_size = 0;
}

std::vector<std::string> CodeCache::getHotFunctions() const {
    std::lock_guard<std::mutex> lock(cache_mutex);
    std::vector<std::string> hot_functions;
    
    for (const auto& [name, info] : cache) {
        if (info.is_hot) {
            hot_functions.push_back(name);
        }
    }
    
    return hot_functions;
}

JITStats CodeCache::getStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex);
    JITStats stats;
    stats.functions_compiled = cache.size();
    stats.cache_hits = 0; // Would need tracking
    stats.cache_misses = 0; // Would need tracking
    return stats;
}

void CodeCache::evictLRU() {
    if (cache.empty()) return;
    
    // Simple LRU: remove the function with lowest call count
    auto min_it = std::min_element(cache.begin(), cache.end(),
        [](const auto& a, const auto& b) {
            return a.second.call_count < b.second.call_count;
        });
    
    if (min_it != cache.end()) {
        current_cache_size -= min_it->second.compiled_code.size();
        cache.erase(min_it);
    }
}

bool CodeCache::shouldEvict() const {
    return current_cache_size > max_cache_size || cache.size() > 1000;
}

// HotSpotProfiler implementation
void HotSpotProfiler::recordFunctionCall(const std::string& function_name, uint64_t execution_time_ns) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(profile_mutex);
    HotFunctionInfo& info = profile_data[function_name];
    info.function_name = function_name;
    info.call_count++;
    info.execution_time_ns += execution_time_ns;
}

void HotSpotProfiler::recordFunctionCompilation(const std::string& function_name, void* compiled_function) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(profile_mutex);
    auto it = profile_data.find(function_name);
    if (it != profile_data.end()) {
        it->second.is_compiled = true;
        it->second.compiled_function = compiled_function;
    }
}

std::vector<std::string> HotSpotProfiler::getHotFunctions(size_t threshold) const {
    std::lock_guard<std::mutex> lock(profile_mutex);
    std::vector<std::string> hot_functions;
    
    for (const auto& [name, info] : profile_data) {
        if (isHot(info, threshold)) {
            hot_functions.push_back(name);
        }
    }
    
    return hot_functions;
}

HotFunctionInfo* HotSpotProfiler::getFunctionInfo(const std::string& function_name) {
    std::lock_guard<std::mutex> lock(profile_mutex);
    auto it = profile_data.find(function_name);
    return it != profile_data.end() ? &it->second : nullptr;
}

void HotSpotProfiler::reset() {
    std::lock_guard<std::mutex> lock(profile_mutex);
    profile_data.clear();
}

JITStats HotSpotProfiler::getStats() const {
    std::lock_guard<std::mutex> lock(profile_mutex);
    JITStats stats;
    
    for (const auto& [name, info] : profile_data) {
        if (info.is_compiled) {
            stats.functions_compiled++;
        }
        stats.execution_time_ms += info.execution_time_ns / 1'000'000;
    }
    
    return stats;
}

bool HotSpotProfiler::isHot(const HotFunctionInfo& info, size_t threshold) const {
    return info.call_count >= threshold || 
           info.execution_time_ns >= threshold * 1000; // Convert to nanoseconds
}

// JITCompiler implementation
JITCompiler::JITCompiler(const JITContext& context) 
    : context(context), llvm_context(nullptr), llvm_module(nullptr), llvm_execution_engine(nullptr) {
    code_cache = std::make_unique<CodeCache>();
    profiler = std::make_unique<HotSpotProfiler>(context.isProfilingEnabled());
    initializeLLVM();
}

JITCompiler::~JITCompiler() {
    shutdownLLVM();
}

void* JITCompiler::compileFunction(const std::string& function_name, 
                                   const std::vector<IRInstruction>& ir_code) {
    std::lock_guard<std::mutex> lock(compiler_mutex);
    
    // Check if already compiled
    if (auto* cached = code_cache->get(function_name)) {
        if (cached->is_compiled) {
            stats.cache_hits++;
            return cached->compiled_function;
        }
    }
    
    stats.cache_misses++;
    
    // Check if we should compile this function
    if (!shouldCompile(function_name)) {
        return nullptr;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Optimize IR
    std::vector<IRInstruction> optimized_ir = ir_code;
    optimizeIR(optimized_ir);
    
    // Compile to native code
    void* compiled_function = compileIRToNative(optimized_ir);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto compilation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (compiled_function) {
        // Cache the compiled function
        HotFunctionInfo info;
        info.function_name = function_name;
        info.is_compiled = true;
        info.compiled_function = compiled_function;
        info.opt_level = context.getOptimizationLevel();
        
        code_cache->put(function_name, info);
        profiler->recordFunctionCompilation(function_name, compiled_function);
        
        updateStats(function_name, compilation_time.count());
    }
    
    return compiled_function;
}

Value JITCompiler::executeFunction(const std::string& function_name, 
                                   const std::vector<Value>& args) {
    auto* compiled_func = getCompiledFunction(function_name);
    if (!compiled_func) {
        return Value(); // Return None if not compiled
    }
    
    // Execute compiled function
    typedef Value (*CompiledFunctionType)(const Value*, size_t);
    auto func = reinterpret_cast<CompiledFunctionType>(compiled_func);
    
    std::vector<Value> arg_values = args;
    return func(arg_values.data(), arg_values.size());
}

void JITCompiler::profileFunctionCall(const std::string& function_name, 
                                      uint64_t execution_time_ns) {
    if (context.isProfilingEnabled()) {
        profiler->recordFunctionCall(function_name, execution_time_ns);
    }
}

bool JITCompiler::isFunctionCompiled(const std::string& function_name) const {
    return code_cache->has(function_name);
}

void* JITCompiler::getCompiledFunction(const std::string& function_name) {
    auto* info = code_cache->get(function_name);
    return info ? info->compiled_function : nullptr;
}

void JITCompiler::invalidateFunction(const std::string& function_name) {
    code_cache->remove(function_name);
}

void JITCompiler::clearCache() {
    code_cache->clear();
}

void JITCompiler::setContext(const JITContext& new_context) {
    context = new_context;
    profiler->enable(new_context.isProfilingEnabled());
}

JITStats JITCompiler::getStats() const {
    JITStats combined_stats = stats;
    JITStats cache_stats = code_cache->getStats();
    JITStats profiler_stats = profiler->getStats();
    
    combined_stats.functions_compiled = cache_stats.functions_compiled;
    combined_stats.cache_hits = cache_stats.cache_hits;
    combined_stats.cache_misses = cache_stats.cache_misses;
    combined_stats.execution_time_ms = profiler_stats.execution_time_ms;
    
    return combined_stats;
}

void JITCompiler::resetStats() {
    stats = JITStats{};
    profiler->reset();
}

void JITCompiler::optimizeIR(std::vector<IRInstruction>& ir_code) {
    OptimizationLevel level = context.getOptimizationLevel();
    
    if (level >= OptimizationLevel::O1) {
        optimizeConstants(ir_code);
        optimizeDeadCode(ir_code);
    }
    
    if (level >= OptimizationLevel::O2) {
        optimizeCommonSubexpressions(ir_code);
        optimizeLoopInvariants(ir_code);
        optimizeTailCalls(ir_code);
    }
    
    if (level >= OptimizationLevel::O3) {
        optimizeInlining(ir_code);
    }
}

void JITCompiler::applyOptimizations(HotFunctionInfo& info) {
    // Apply additional optimizations based on hotness
    if (info.call_count > 10000) {
        info.opt_level = OptimizationLevel::O3;
    } else if (info.call_count > 1000) {
        info.opt_level = OptimizationLevel::O2;
    } else {
        info.opt_level = OptimizationLevel::O1;
    }
}

void JITCompiler::initializeLLVM() {
    // Simplified LLVM initialization
    // In a real implementation, this would initialize LLVM JIT infrastructure
    llvm_context = reinterpret_cast<void*>(0x1); // Placeholder
    llvm_module = reinterpret_cast<void*>(0x2);   // Placeholder
    llvm_execution_engine = reinterpret_cast<void*>(0x3); // Placeholder
}

void JITCompiler::shutdownLLVM() {
    // Simplified LLVM cleanup
    llvm_context = nullptr;
    llvm_module = nullptr;
    llvm_execution_engine = nullptr;
}

void* JITCompiler::compileIRToNative(const std::vector<IRInstruction>& ir_code) {
    // Simplified compilation - in reality this would use LLVM
    std::vector<uint8_t> native_code = generateNativeCode(ir_code);
    return createExecutableMemory(native_code);
}

void JITCompiler::optimizeConstants(std::vector<IRInstruction>& ir_code) {
    // Constant folding optimization
    for (auto& instr : ir_code) {
        if (instr.opcode == IROpcode::ADD && 
            instr.operand1.isConstant() && instr.operand2.isConstant()) {
            // Fold constant addition
            int result = instr.operand1.asInt() + instr.operand2.asInt();
            instr = IRInstruction(IROpcode::LOAD_CONST, Value(result));
        }
        // Add more constant folding cases...
    }
}

void JITCompiler::optimizeDeadCode(std::vector<IRInstruction>& ir_code) {
    // Dead code elimination
    std::vector<IRInstruction> optimized;
    std::unordered_set<int> used_vars;
    
    // First pass: find used variables
    for (const auto& instr : ir_code) {
        if (instr.operand1.isVariable()) {
            used_vars.insert(instr.operand1.asInt());
        }
        if (instr.operand2.isVariable()) {
            used_vars.insert(instr.operand2.asInt());
        }
    }
    
    // Second pass: keep only used instructions
    for (const auto& instr : ir_code) {
        if (instr.result.isVariable() || 
            instr.opcode == IROpcode::RETURN ||
            instr.opcode == IROpcode::CALL) {
            optimized.push_back(instr);
        }
    }
    
    ir_code = std::move(optimized);
}

void JITCompiler::optimizeCommonSubexpressions(std::vector<IRInstruction>& ir_code) {
    // Common subexpression elimination
    std::unordered_map<std::string, int> expr_map;
    
    for (auto& instr : ir_code) {
        if (instr.opcode == IROpcode::ADD || instr.opcode == IROpcode::MUL) {
            std::string expr_key = std::to_string(static_cast<int>(instr.opcode)) + ":" +
                                  instr.operand1.toString() + ":" + instr.operand2.toString();
            
            auto it = expr_map.find(expr_key);
            if (it != expr_map.end()) {
                // Replace with previous result
                instr = IRInstruction(IROpcode::MOVE, Value(it->second), instr.result);
            } else {
                expr_map[expr_key] = instr.result.asInt();
            }
        }
    }
}

void JITCompiler::optimizeLoopInvariants(std::vector<IRInstruction>& ir_code) {
    // Loop invariant code motion
    // Simplified implementation
    for (size_t i = 0; i < ir_code.size(); ++i) {
        if (ir_code[i].opcode == IROpcode::ADD && 
            ir_code[i].operand1.isConstant() && 
            ir_code[i].operand2.isConstant()) {
            // Move constant operations out of loops
            // This is a simplified version
        }
    }
}

void JITCompiler::optimizeTailCalls(std::vector<IRInstruction>& ir_code) {
    // Tail call optimization
    for (size_t i = 0; i < ir_code.size(); ++i) {
        if (ir_code[i].opcode == IROpcode::CALL && 
            i + 1 < ir_code.size() && 
            ir_code[i + 1].opcode == IROpcode::RETURN) {
            // Convert to tail call
            ir_code[i].opcode = IROpcode::TAIL_CALL;
        }
    }
}

void JITCompiler::optimizeInlining(std::vector<IRInstruction>& ir_code) {
    // Function inlining
    // Simplified implementation
    for (auto& instr : ir_code) {
        if (instr.opcode == IROpcode::CALL) {
            // Check if function should be inlined
            // This is a simplified version
        }
    }
}

std::vector<uint8_t> JITCompiler::generateNativeCode(const std::vector<IRInstruction>& ir_code) {
    // Simplified native code generation
    // In reality, this would use LLVM or a similar backend
    std::vector<uint8_t> code;
    
    // Add simple machine code placeholder
    for (const auto& instr : ir_code) {
        switch (instr.opcode) {
            case IROpcode::LOAD_CONST:
                code.push_back(0xB8); // MOV EAX, imm32
                // Add constant value
                break;
            case IROpcode::ADD:
                code.push_back(0x01); // ADD
                break;
            case IROpcode::RETURN:
                code.push_back(0xC3); // RET
                break;
            default:
                break;
        }
    }
    
    return code;
}

void* JITCompiler::createExecutableMemory(const std::vector<uint8_t>& code) {
    if (code.empty()) return nullptr;
    
    // Allocate executable memory
#ifdef _WIN32
    void* mem = VirtualAlloc(nullptr, code.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    void* mem = mmap(nullptr, code.size(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    
    if (mem) {
        std::memcpy(mem, code.data(), code.size());
    }
    
    return mem;
}

bool JITCompiler::shouldCompile(const std::string& function_name) const {
    if (context.getMode() == JITMode::INTERPRETED) {
        return false;
    }
    
    if (context.getMode() == JITMode::COMPILED) {
        return true;
    }
    
    // Mixed mode: check if function is hot
    auto* info = profiler->getFunctionInfo(function_name);
    return info && info->call_count >= context.getCompilationThreshold();
}

OptimizationLevel JITCompiler::determineOptimizationLevel(const HotFunctionInfo& info) const {
    if (info.call_count > 10000) {
        return OptimizationLevel::O3;
    } else if (info.call_count > 1000) {
        return OptimizationLevel::O2;
    } else if (info.call_count > 100) {
        return OptimizationLevel::O1;
    }
    return OptimizationLevel::O0;
}

void JITCompiler::updateStats(const std::string& function_name, uint64_t compilation_time_ms) {
    stats.functions_compiled++;
    stats.compilation_time_ms += compilation_time_ms;
}

// JITExecutionEngine implementation
JITExecutionEngine::JITExecutionEngine(Runtime* runtime, bool jit_enabled)
    : runtime(runtime), jit_enabled(jit_enabled) {
    compiler = std::make_unique<JITCompiler>();
}

JITExecutionEngine::~JITExecutionEngine() = default;

Value JITExecutionEngine::executeFunction(const std::string& function_name,
                                          const std::vector<Value>& args,
                                          const std::vector<IRInstruction>& ir_code) {
    if (!jit_enabled) {
        return executeInterpreted(ir_code, args);
    }
    
    // Try to get compiled function
    auto* compiled_func = compiler->getCompiledFunction(function_name);
    if (compiled_func) {
        return executeCompiled(compiled_func, args);
    }
    
    // Profile the interpreted execution
    auto start_time = std::chrono::high_resolution_clock::now();
    Value result = executeInterpreted(ir_code, args);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto execution_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    compiler->profileFunctionCall(function_name, execution_time.count());
    
    // Check if we should compile this function
    if (compiler->shouldCompile(function_name)) {
        compiler->compileFunction(function_name, ir_code);
    }
    
    return result;
}

Value JITExecutionEngine::executeModule(const std::string& module_name,
                                        const std::vector<IRInstruction>& ir_code) {
    // Execute module with JIT compilation
    return executeInterpreted(ir_code, {});
}

void JITExecutionEngine::setJITContext(const JITContext& context) {
    compiler->setContext(context);
}

JITStats JITExecutionEngine::getJITStats() const {
    return compiler->getStats();
}

void JITExecutionEngine::resetJITStats() {
    compiler->resetStats();
}

Value JITExecutionEngine::executeInterpreted(const std::vector<IRInstruction>& ir_code,
                                             const std::vector<Value>& args) {
    // Simplified interpreter
    std::unordered_map<int, Value> variables;
    
    // Set up arguments
    for (size_t i = 0; i < args.size(); ++i) {
        variables[i] = args[i];
    }
    
    Value last_result;
    
    for (const auto& instr : ir_code) {
        switch (instr.opcode) {
            case IROpcode::LOAD_CONST:
                variables[instr.result.asInt()] = instr.operand1;
                break;
            case IROpcode::ADD:
                if (instr.operand1.isVariable() && instr.operand2.isVariable()) {
                    int var1 = instr.operand1.asInt();
                    int var2 = instr.operand2.asInt();
                    int result = variables[var1].asInt() + variables[var2].asInt();
                    variables[instr.result.asInt()] = Value(result);
                }
                break;
            case IROpcode::RETURN:
                last_result = instr.operand1;
                break;
            default:
                break;
        }
    }
    
    return last_result;
}

Value JITExecutionEngine::executeCompiled(void* compiled_function, const std::vector<Value>& args) {
    typedef Value (*CompiledFunctionType)(const Value*, size_t);
    auto func = reinterpret_cast<CompiledFunctionType>(compiled_function);
    
    std::vector<Value> arg_values = args;
    return func(arg_values.data(), arg_values.size());
}

uint64_t JITExecutionEngine::measureExecutionTime(std::function<void()> func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

// JITManager implementation
JITManager::JITManager(Runtime* runtime) 
    : initialized(false) {
    execution_engine = std::make_unique<JITExecutionEngine>(runtime);
    setupDefaultOptimizationPasses();
}

JITManager::~JITManager() {
    shutdown();
}

bool JITManager::initialize() {
    if (initialized) return true;
    
    // Initialize JIT components
    initialized = true;
    return true;
}

void JITManager::shutdown() {
    if (!initialized) return;
    
    clearJITCache();
    initialized = false;
}

Value JITManager::executeFunction(const std::string& function_name,
                                  const std::vector<Value>& args,
                                  const std::vector<IRInstruction>& ir_code) {
    if (!initialized) {
        return Value(); // Return None if not initialized
    }
    
    return execution_engine->executeFunction(function_name, args, ir_code);
}

void JITManager::setJITContext(const JITContext& context) {
    execution_engine->setJITContext(context);
}

void JITManager::addOptimizationPass(std::unique_ptr<JITPass> pass) {
    optimization_passes.push_back(std::move(pass));
}

JITStats JITManager::getStats() const {
    return execution_engine->getJITStats();
}

void JITManager::resetStats() {
    execution_engine->resetJITStats();
}

void JITManager::printStats() const {
    JITStats stats = getStats();
    std::cout << "JIT Statistics:\n";
    std::cout << "  Functions compiled: " << stats.functions_compiled << "\n";
    std::cout << "  Compilation time: " << stats.compilation_time_ms << " ms\n";
    std::cout << "  Execution time: " << stats.execution_time_ms << " ms\n";
    std::cout << "  Cache hits: " << stats.cache_hits << "\n";
    std::cout << "  Cache misses: " << stats.cache_misses << "\n";
    std::cout << "  Speedup factor: " << stats.speedup_factor << "x\n";
}

void JITManager::clearJITCache() {
    execution_engine->getCompiler()->clearCache();
}

void JITManager::invalidateFunction(const std::string& function_name) {
    execution_engine->getCompiler()->invalidateFunction(function_name);
}

void JITManager::setupDefaultOptimizationPasses() {
    optimization_passes.clear();
    
    // Add default optimization passes in order
    optimization_passes.push_back(std::make_unique<ConstantFoldingPass>());
    optimization_passes.push_back(std::make_unique<DeadCodeEliminationPass>());
    optimization_passes.push_back(std::make_unique<CommonSubexpressionEliminationPass>());
    optimization_passes.push_back(std::make_unique<LoopInvariantCodeMotionPass>());
    optimization_passes.push_back(std::make_unique<TailCallOptimizationPass>());
    optimization_passes.push_back(std::make_unique<InliningPass>());
}

} // namespace pyplusplus