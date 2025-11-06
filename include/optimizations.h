#pragma once
#include "ir.h"
#include "aot.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace pyplusplus {

// Forward declarations
class OptimizationPass;
class OptimizationContext;
class InliningOptimizer;
class VectorizationOptimizer;
class EscapeAnalysisOptimizer;

// Optimization pass types
enum class PassType {
    FUNCTION_PASS,      // Operates on individual functions
    MODULE_PASS,        // Operates on entire module
    LOOP_PASS,          // Operates on loops
    REGION_PASS         // Operates on code regions
};

// Optimization levels
enum class OptimizationLevel {
    O0,   // No optimization
    O1,   // Basic optimizations
    O2,   // Standard optimizations
    O3,   // Aggressive optimizations
    Os,   // Optimize for size
    Oz    // Optimize for size aggressively
};

// Optimization result
struct OptimizationResult {
    bool success = false;
    size_t instructions_removed = 0;
    size_t instructions_added = 0;
    size_t loops_optimized = 0;
    size_t functions_inlined = 0;
    size_t memory_saved = 0;
    double speedup_factor = 1.0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

// Control flow analysis
class ControlFlowAnalysis {
public:
    struct BasicBlock {
        std::vector<std::unique_ptr<IRInstruction>> instructions;
        std::vector<BasicBlock*> predecessors;
        std::vector<BasicBlock*> successors;
        bool is_entry = false;
        bool is_exit = false;
        int id = -1;
    };
    
    struct ControlFlowGraph {
        std::vector<std::unique_ptr<BasicBlock>> blocks;
        BasicBlock* entry_block = nullptr;
        BasicBlock* exit_block = nullptr;
    };
    
    static std::unique_ptr<ControlFlowGraph> buildCFG(IRFunction* function);
    static std::vector<BasicBlock*> findDominators(const ControlFlowGraph& cfg);
    static std::vector<BasicBlock*> findLoops(const ControlFlowGraph& cfg);
};

// Advanced optimization passes
class LoopVectorizer {
public:
    struct VectorizationInfo {
        bool can_vectorize = false;
        int vector_width = 4;
        std::vector<IRInstruction*> vectorizable_ops;
        std::vector<IRInstruction*> reduction_ops;
    };
    
    static VectorizationInfo analyzeLoop(BasicBlock* loop_header);
    static OptimizationResult vectorizeLoop(BasicBlock* loop_header, const VectorizationInfo& info);
};

class MemoryOptimizer {
public:
    struct MemoryAccessPattern {
        bool is_sequential = false;
        bool is_random = false;
        int stride = 0;
        size_t access_count = 0;
        std::vector<IRInstruction*> memory_ops;
    };
    
    static std::vector<MemoryAccessPattern> analyzeMemoryAccess(IRFunction* function);
    static OptimizationResult optimizeMemoryLayout(IRFunction* function, 
                                              const std::vector<MemoryAccessPattern>& patterns);
    static OptimizationResult insertPrefetches(IRFunction* function, 
                                           const std::vector<MemoryAccessPattern>& patterns);
};

class ConstantPropagation {
public:
    struct ConstantValue {
        bool is_constant = false;
        Value value;
        IRInstruction* definition = nullptr;
    };
    
    static std::unordered_map<IRInstruction*, ConstantValue> analyzeConstants(IRFunction* function);
    static OptimizationResult propagateConstants(IRFunction* function, 
                                            const std::unordered_map<IRInstruction*, ConstantValue>& constants);
};

class DeadCodeElimination {
public:
    struct LivenessInfo {
        std::unordered_set<IRInstruction*> live_instructions;
        std::unordered_set<IRInstruction*> dead_instructions;
    };
    
    static LivenessInfo analyzeLiveness(IRFunction* function);
    static OptimizationResult eliminateDeadCode(IRFunction* function, const LivenessInfo& liveness);
};

class FunctionSpecializer {
public:
    struct SpecializationKey {
        std::vector<Value> constant_args;
        std::vector<std::string> type_hints;
        bool operator==(const SpecializationKey& other) const;
        bool operator<(const SpecializationKey& other) const;
    };
    
    struct SpecializedFunction {
        IRFunction* specialized_func;
        SpecializationKey key;
        double performance_gain;
    };
    
    static std::vector<SpecializationKey> findSpecializationOpportunities(IRFunction* function);
    static SpecializedFunction createSpecialization(IRFunction* function, const SpecializationKey& key);
};

class ProfileGuidedOptimizer {
public:
    struct ProfileData {
        std::unordered_map<IRInstruction*, uint64_t> execution_counts;
        std::unordered_map<BasicBlock*, uint64_t> block_frequencies;
        std::unordered_map<std::string, double> function_times;
        std::vector<std::pair<IRInstruction*, double>> hot_paths;
    };
    
    static ProfileData collectProfileData(IRModule* module);
    static OptimizationResult optimizeBasedOnProfile(IRModule* module, const ProfileData& profile);
};

class InterproceduralOptimizer {
public:
    struct CallGraph {
        std::unordered_map<IRFunction*, std::vector<IRFunction*>> callers;
        std::unordered_map<IRFunction*, std::vector<IRFunction*>> callees;
        std::unordered_set<IRFunction*> recursive_functions;
    };
    
    static CallGraph buildCallGraph(IRModule* module);
    static OptimizationResult optimizeCallGraph(IRModule* module, const CallGraph& callgraph);
    static OptimizationResult devirtualizeCalls(IRModule* module, const CallGraph& callgraph);
};

class ParallelizationOptimizer {
public:
    struct ParallelRegion {
        BasicBlock* entry;
        BasicBlock* exit;
        std::vector<IRInstruction*> independent_ops;
        bool has_dependencies = false;
    };
    
    static std::vector<ParallelRegion> findParallelRegions(IRFunction* function);
    static OptimizationResult insertParallelCode(IRFunction* function, 
                                             const std::vector<ParallelRegion>& regions);
};
    
    std::vector<BasicBlock> blocks;
    std::unordered_map<size_t, size_t> instruction_to_block;
    
    void buildFromIR(const std::vector<IRInstruction>& instructions);
    std::vector<size_t> getLoopBlocks(size_t header_id) const;
    bool isInLoop(size_t block_id, size_t loop_header) const;
    void calculateDominators();
    std::unordered_set<size_t> getDominators(size_t block_id) const;
};

// Data flow analysis
class DataFlowAnalysis {
public:
    using Domain = std::unordered_set<int>; // Variable IDs
    using TransferFunction = std::function<Domain(const Domain&, const IRInstruction&)>;
    
    struct AnalysisResult {
        std::unordered_map<size_t, Domain> in_sets;   // Block ID -> IN set
        std::unordered_map<size_t, Domain> out_sets;  // Block ID -> OUT set
        std::unordered_map<size_t, Domain> gen_sets;   // Block ID -> GEN set
        std::unordered_map<size_t, Domain> kill_sets;  // Block ID -> KILL set
    };
    
    virtual ~DataFlowAnalysis() = default;
    virtual AnalysisResult analyze(const ControlFlowGraph& cfg) = 0;
    virtual TransferFunction getTransferFunction() = 0;
    virtual bool isForward() const = 0;
};

// Reaching definitions analysis
class ReachingDefinitionsAnalysis : public DataFlowAnalysis {
public:
    AnalysisResult analyze(const ControlFlowGraph& cfg) override;
    TransferFunction getTransferFunction() override;
    bool isForward() const override { return true; }
};

// Live variable analysis
class LiveVariableAnalysis : public DataFlowAnalysis {
public:
    AnalysisResult analyze(const ControlFlowGraph& cfg) override;
    TransferFunction getTransferFunction() override;
    bool isForward() const override { return false; }
};

// Constant propagation analysis
class ConstantPropagationAnalysis : public DataFlowAnalysis {
public:
    struct LatticeValue {
        bool is_constant = false;
        Value constant_value;
        bool is_top = true;  // Unknown
        bool is_bottom = false; // Contradiction
        
        static LatticeValue top() { return LatticeValue{false, Value{}, true, false}; }
        static LatticeValue bottom() { return LatticeValue{false, Value{}, false, true}; }
        static LatticeValue constant(const Value& val) { return LatticeValue{true, val, false, false}; }
    };
    
    using Domain = std::unordered_map<int, LatticeValue>; // Variable ID -> Lattice value
    
    AnalysisResult analyze(const ControlFlowGraph& cfg) override;
    TransferFunction getTransferFunction() override;
    bool isForward() const override { return true; }
};

// Base optimization pass
class OptimizationPass {
protected:
    OptimizationLevel level;
    bool enabled;
    std::string name;
    
public:
    OptimizationPass(const std::string& name, OptimizationLevel level = OptimizationLevel::O2)
        : level(level), enabled(true), name(name) {}
    
    virtual ~OptimizationPass() = default;
    
    virtual OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                                 OptimizationContext& context) = 0;
    
    virtual bool shouldRun(OptimizationLevel current_level) const {
        return enabled && current_level >= level;
    }
    
    const std::string& getName() const { return name; }
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    OptimizationLevel getLevel() const { return level; }
};

// Function inlining optimizer
class InliningOptimizer : public OptimizationPass {
private:
    size_t max_inline_size;
    double inline_cost_threshold;
    std::unordered_map<std::string, std::vector<IRInstruction>> function_bodies;
    
public:
    InliningOptimizer(size_t max_size = 50, double cost_threshold = 10.0)
        : OptimizationPass("Function Inlining", OptimizationLevel::O2),
          max_inline_size(max_size), inline_cost_threshold(cost_threshold) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
    void setFunctionBodies(const std::unordered_map<std::string, std::vector<IRInstruction>>& bodies) {
        function_bodies = bodies;
    }
    
private:
    bool shouldInline(const std::string& function_name, const std::vector<IRInstruction>& body);
    size_t estimateInlineCost(const std::vector<IRInstruction>& body);
    std::vector<IRInstruction> inlineFunction(const std::vector<IRInstruction>& caller,
                                           const std::vector<IRInstruction>& callee,
                                           size_t call_site);
};

// Loop vectorization optimizer
class VectorizationOptimizer : public OptimizationPass {
private:
    size_t vector_width;
    bool enable_simd;
    
public:
    VectorizationOptimizer(size_t width = 4, bool simd = true)
        : OptimizationPass("Loop Vectorization", OptimizationLevel::O3),
          vector_width(width), enable_simd(simd) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    struct LoopInfo {
        size_t start_index;
        size_t end_index;
        int induction_variable;
        std::vector<IRInstruction> body;
        bool is_vectorizable = false;
    };
    
    std::vector<LoopInfo> findLoops(const std::vector<IRInstruction>& ir_code);
    bool isLoopVectorizable(const LoopInfo& loop);
    std::vector<IRInstruction> vectorizeLoop(const LoopInfo& loop);
    std::vector<IRInstruction> generateVectorizedOperation(const IRInstruction& instr, size_t width);
};

// Escape analysis optimizer
class EscapeAnalysisOptimizer : public OptimizationPass {
private:
    struct EscapeInfo {
        bool escapes = false;
        bool escapes_to_call = false;
        bool escapes_to_return = false;
        std::unordered_set<int> escape_points; // Instruction indices where it escapes
    };
    
    std::unordered_map<int, EscapeInfo> escape_analysis; // Variable ID -> Escape info
    
public:
    EscapeAnalysisOptimizer() : OptimizationPass("Escape Analysis", OptimizationLevel::O2) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    void analyzeEscapes(const std::vector<IRInstruction>& ir_code);
    EscapeInfo analyzeVariable(int var_id, const std::vector<IRInstruction>& ir_code);
    bool doesEscape(int var_id, const std::vector<IRInstruction>& ir_code);
    std::vector<IRInstruction> optimizeAllocations(const std::vector<IRInstruction>& ir_code);
};

// Dead code elimination
class DeadCodeEliminationPass : public OptimizationPass {
public:
    DeadCodeEliminationPass() : OptimizationPass("Dead Code Elimination", OptimizationLevel::O1) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    std::unordered_set<int> findLiveVariables(const std::vector<IRInstruction>& ir_code);
    std::vector<IRInstruction> removeDeadCode(const std::vector<IRInstruction>& ir_code,
                                             const std::unordered_set<int>& live_vars);
};

// Constant folding
class ConstantFoldingPass : public OptimizationPass {
public:
    ConstantFoldingPass() : OptimizationPass("Constant Folding", OptimizationLevel::O1) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    Value evaluateConstantExpression(const IRInstruction& instr);
    bool isConstant(const Value& val);
    IRInstruction foldInstruction(const IRInstruction& instr);
};

// Common subexpression elimination
class CommonSubexpressionEliminationPass : public OptimizationPass {
public:
    CommonSubexpressionEliminationPass() : OptimizationPass("Common Subexpression Elimination", OptimizationLevel::O2) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    std::unordered_map<std::string, int> expression_map;
    std::string getExpressionKey(const IRInstruction& instr);
};

// Loop invariant code motion
class LoopInvariantCodeMotionPass : public OptimizationPass {
public:
    LoopInvariantCodeMotionPass() : OptimizationPass("Loop Invariant Code Motion", OptimizationLevel::O2) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    struct LoopInfo {
        size_t start;
        size_t end;
        std::unordered_set<int> loop_variables;
    };
    
    std::vector<LoopInfo> findLoops(const std::vector<IRInstruction>& ir_code);
    bool isInvariant(const IRInstruction& instr, const LoopInfo& loop);
    std::vector<IRInstruction> moveInvariants(const std::vector<IRInstruction>& ir_code,
                                            const std::vector<LoopInfo>& loops);
};

// Tail call optimization
class TailCallOptimizationPass : public OptimizationPass {
public:
    TailCallOptimizationPass() : OptimizationPass("Tail Call Optimization", OptimizationLevel::O2) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    bool isTailCall(const IRInstruction& instr, size_t index, const std::vector<IRInstruction>& ir_code);
    std::vector<IRInstruction> optimizeTailCalls(const std::vector<IRInstruction>& ir_code);
};

// Strength reduction
class StrengthReductionPass : public OptimizationPass {
public:
    StrengthReductionPass() : OptimizationPass("Strength Reduction", OptimizationLevel::O1) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    IRInstruction reduceStrength(const IRInstruction& instr);
};

// Peephole optimization
class PeepholeOptimizationPass : public OptimizationPass {
private:
    size_t window_size;
    
public:
    PeepholeOptimizationPass(size_t window = 3) 
        : OptimizationPass("Peephole Optimization", OptimizationLevel::O1), window_size(window) {}
    
    OptimizationResult run(std::vector<IRInstruction>& ir_code, 
                         OptimizationContext& context) override;
    
private:
    std::vector<IRInstruction> optimizeWindow(const std::vector<IRInstruction>& window);
    bool canOptimizeWindow(const std::vector<IRInstruction>& window);
};

// Optimization context
class OptimizationContext {
private:
    OptimizationLevel level;
    std::unordered_map<std::string, std::any> analysis_results;
    std::vector<std::unique_ptr<OptimizationPass>> passes;
    ControlFlowGraph cfg;
    bool verbose;
    
public:
    OptimizationContext(OptimizationLevel level = OptimizationLevel::O2, bool verbose = false)
        : level(level), verbose(verbose) {}
    
    void addPass(std::unique_ptr<OptimizationPass> pass);
    void setupDefaultPasses();
    
    OptimizationResult runOptimizations(std::vector<IRInstruction>& ir_code);
    
    template<typename T>
    void setAnalysisResult(const std::string& name, const T& result) {
        analysis_results[name] = result;
    }
    
    template<typename T>
    T getAnalysisResult(const std::string& name) const {
        auto it = analysis_results.find(name);
        if (it != analysis_results.end()) {
            return std::any_cast<T>(it->second);
        }
        return T{};
    }
    
    void setControlFlowGraph(const ControlFlowGraph& new_cfg) { cfg = new_cfg; }
    const ControlFlowGraph& getControlFlowGraph() const { return cfg; }
    
    OptimizationLevel getLevel() const { return level; }
    void setLevel(OptimizationLevel new_level) { level = new_level; }
    
    bool isVerbose() const { return verbose; }
    void setVerbose(bool v) { verbose = v; }
    
    const std::vector<std::unique_ptr<OptimizationPass>>& getPasses() const { return passes; }
};

// Optimization manager
class OptimizationManager {
private:
    std::unique_ptr<OptimizationContext> context;
    std::unordered_map<std::string, OptimizationResult> results;
    
public:
    OptimizationManager(OptimizationLevel level = OptimizationLevel::O2, bool verbose = false);
    
    void optimizeFunction(const std::string& name, std::vector<IRInstruction>& ir_code);
    void optimizeModule(std::unordered_map<std::string, std::vector<IRInstruction>>& functions);
    
    void addCustomPass(std::unique_ptr<OptimizationPass> pass);
    void setOptimizationLevel(OptimizationLevel level);
    
    OptimizationResult getFunctionResult(const std::string& name) const;
    std::unordered_map<std::string, OptimizationResult> getAllResults() const;
    
    void printOptimizationReport() const;
    void reset();
    
private:
    void runDataFlowAnalyses(const std::vector<IRInstruction>& ir_code);
    void updateControlFlowGraph(const std::vector<IRInstruction>& ir_code);
};

// Performance profiler for optimizations
class OptimizationProfiler {
private:
    struct PassProfile {
        std::string pass_name;
        size_t execution_count = 0;
        uint64_t total_time_ns = 0;
        size_t total_instructions_removed = 0;
        size_t total_instructions_added = 0;
        double total_speedup = 0.0;
    };
    
    std::unordered_map<std::string, PassProfile> profiles;
    bool enabled;
    
public:
    OptimizationProfiler(bool enabled = true) : enabled(enabled) {}
    
    void startPass(const std::string& pass_name);
    void endPass(const std::string& pass_name, const OptimizationResult& result);
    
    PassProfile getProfile(const std::string& pass_name) const;
    std::unordered_map<std::string, PassProfile> getAllProfiles() const;
    
    void printProfileReport() const;
    void reset();
    
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    
private:
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_times;
};

} // namespace pyplusplus