#include "optimizations.h"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace pyplusplus {

// ControlFlowGraph implementation
void ControlFlowGraph::buildFromIR(const std::vector<IRInstruction>& instructions) {
    blocks.clear();
    instruction_to_block.clear();
    
    // Create basic blocks
    size_t current_block_id = 0;
    std::vector<IRInstruction> current_block_instructions;
    
    for (size_t i = 0; i < instructions.size(); ++i) {
        const auto& instr = instructions[i];
        
        // Start new block at labels and after branches
        if (instr.opcode == IROpcode::LABEL || 
            (i > 0 && (instructions[i-1].opcode == IROpcode::JUMP ||
                       instructions[i-1].opcode == IROpcode::JUMP_IF_TRUE ||
                       instructions[i-1].opcode == IROpcode::JUMP_IF_FALSE ||
                       instructions[i-1].opcode == IROpcode::RETURN))) {
            
            if (!current_block_instructions.empty()) {
                blocks.push_back({current_block_id++, std::move(current_block_instructions), {}, {}, false, false});
                current_block_instructions.clear();
            }
        }
        
        current_block_instructions.push_back(instr);
        instruction_to_block[i] = current_block_id;
    }
    
    // Add final block
    if (!current_block_instructions.empty()) {
        blocks.push_back({current_block_id++, std::move(current_block_instructions), {}, {}, false, false});
    }
    
    // Build successor/predecessor relationships
    for (size_t block_id = 0; block_id < blocks.size(); ++block_id) {
        auto& block = blocks[block_id];
        
        if (!block.instructions.empty()) {
            const auto& last_instr = block.instructions.back();
            
            if (last_instr.opcode == IROpcode::JUMP) {
                // Unconditional branch
                size_t target_block = instruction_to_block[last_instr.operand1.asInt()];
                block.successors.push_back(target_block);
                blocks[target_block].predecessors.push_back(block_id);
            } else if (last_instr.opcode == IROpcode::JUMP_IF_TRUE || 
                      last_instr.opcode == IROpcode::JUMP_IF_FALSE) {
                // Conditional branch
                size_t target_block = instruction_to_block[last_instr.operand1.asInt()];
                size_t fallthrough_block = block_id + 1;
                
                block.successors.push_back(target_block);
                block.successors.push_back(fallthrough_block);
                
                blocks[target_block].predecessors.push_back(block_id);
                if (fallthrough_block < blocks.size()) {
                    blocks[fallthrough_block].predecessors.push_back(block_id);
                }
            } else if (last_instr.opcode != IROpcode::RETURN) {
                // Fallthrough to next block
                if (block_id + 1 < blocks.size()) {
                    block.successors.push_back(block_id + 1);
                    blocks[block_id + 1].predecessors.push_back(block_id);
                }
            }
        }
    }
    
    // Identify loop headers and exits
    identifyLoops();
}

void ControlFlowGraph::identifyLoops() {
    // Simple loop detection: back edges indicate loops
    for (size_t i = 0; i < blocks.size(); ++i) {
        for (size_t successor : blocks[i].successors) {
            if (successor <= i) {
                // Back edge found - successor is a loop header
                blocks[successor].is_loop_header = true;
                blocks[i].is_loop_exit = true;
            }
        }
    }
}

std::vector<size_t> ControlFlowGraph::getLoopBlocks(size_t header_id) const {
    std::vector<size_t> loop_blocks;
    std::unordered_set<size_t> visited;
    std::vector<size_t> stack;
    
    stack.push_back(header_id);
    
    while (!stack.empty()) {
        size_t current = stack.back();
        stack.pop_back();
        
        if (visited.count(current)) continue;
        visited.insert(current);
        loop_blocks.push_back(current);
        
        for (size_t successor : blocks[current].successors) {
            if (!visited.count(successor) && isInLoop(successor, header_id)) {
                stack.push_back(successor);
            }
        }
    }
    
    return loop_blocks;
}

bool ControlFlowGraph::isInLoop(size_t block_id, size_t loop_header) const {
    if (block_id == loop_header) return true;
    
    std::unordered_set<size_t> visited;
    std::vector<size_t> stack = {block_id};
    
    while (!stack.empty()) {
        size_t current = stack.back();
        stack.pop_back();
        
        if (visited.count(current)) continue;
        visited.insert(current);
        
        if (current == loop_header) return true;
        
        // Don't go past the loop header
        if (current < loop_header) continue;
        
        for (size_t successor : blocks[current].successors) {
            if (!visited.count(successor)) {
                stack.push_back(successor);
            }
        }
    }
    
    return false;
}

// ReachingDefinitionsAnalysis implementation
DataFlowAnalysis::AnalysisResult ReachingDefinitionsAnalysis::analyze(const ControlFlowGraph& cfg) {
    AnalysisResult result;
    
    // Initialize GEN and KILL sets for each block
    for (const auto& block : cfg.blocks) {
        std::unordered_set<int> gen_set, kill_set;
        
        for (const auto& instr : block.instructions) {
            if (instr.result.isVariable()) {
                int var_id = instr.result.asInt();
                
                // This instruction generates a definition for var_id
                gen_set.insert(var_id);
                
                // It kills all other definitions of var_id
                kill_set.insert(var_id);
            }
        }
        
        result.gen_sets[block.id] = gen_set;
        result.kill_sets[block.id] = kill_set;
    }
    
    // Initialize IN and OUT sets
    for (const auto& block : cfg.blocks) {
        result.in_sets[block.id] = {};
        result.out_sets[block.id] = {};
    }
    
    // Fixed-point iteration
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (const auto& block : cfg.blocks) {
            // IN[B] = Union of OUT[P] for all predecessors P of B
            std::unordered_set<int> new_in;
            for (size_t pred_id : block.predecessors) {
                for (int def : result.out_sets.at(pred_id)) {
                    new_in.insert(def);
                }
            }
            
            // OUT[B] = GEN[B] Union (IN[B] - KILL[B])
            std::unordered_set<int> new_out = result.gen_sets.at(block.id);
            for (int def : new_in) {
                if (result.kill_sets.at(block.id).count(def) == 0) {
                    new_out.insert(def);
                }
            }
            
            if (new_in != result.in_sets[block.id] || new_out != result.out_sets[block.id]) {
                result.in_sets[block.id] = new_in;
                result.out_sets[block.id] = new_out;
                changed = true;
            }
        }
    }
    
    return result;
}

DataFlowAnalysis::TransferFunction ReachingDefinitionsAnalysis::getTransferFunction() {
    return [](const Domain& in, const IRInstruction& instr) -> Domain {
        Domain out = in;
        
        if (instr.result.isVariable()) {
            int var_id = instr.result.asInt();
            out.insert(var_id);
        }
        
        return out;
    };
}

// LiveVariableAnalysis implementation
DataFlowAnalysis::AnalysisResult LiveVariableAnalysis::analyze(const ControlFlowGraph& cfg) {
    AnalysisResult result;
    
    // Initialize GEN and KILL sets for each block
    for (const auto& block : cfg.blocks) {
        std::unordered_set<int> gen_set, kill_set;
        
        for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
            const auto& instr = *it;
            
            if (instr.result.isVariable()) {
                int var_id = instr.result.asInt();
                
                // If variable is used before being defined in this block
                if (kill_set.count(var_id) == 0) {
                    gen_set.insert(var_id);
                }
                
                kill_set.insert(var_id);
            }
            
            // Check for variable uses
            if (instr.operand1.isVariable()) {
                int var_id = instr.operand1.asInt();
                if (kill_set.count(var_id) == 0) {
                    gen_set.insert(var_id);
                }
            }
            
            if (instr.operand2.isVariable()) {
                int var_id = instr.operand2.asInt();
                if (kill_set.count(var_id) == 0) {
                    gen_set.insert(var_id);
                }
            }
        }
        
        result.gen_sets[block.id] = gen_set;
        result.kill_sets[block.id] = kill_set;
    }
    
    // Initialize IN and OUT sets
    for (const auto& block : cfg.blocks) {
        result.in_sets[block.id] = {};
        result.out_sets[block.id] = {};
    }
    
    // Fixed-point iteration (backward analysis)
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (auto it = cfg.blocks.rbegin(); it != cfg.blocks.rend(); ++it) {
            const auto& block = *it;
            
            // OUT[B] = Union of IN[S] for all successors S of B
            std::unordered_set<int> new_out;
            for (size_t succ_id : block.successors) {
                for (int var : result.in_sets.at(succ_id)) {
                    new_out.insert(var);
                }
            }
            
            // IN[B] = GEN[B] Union (OUT[B] - KILL[B])
            std::unordered_set<int> new_in = result.gen_sets.at(block.id);
            for (int var : new_out) {
                if (result.kill_sets.at(block.id).count(var) == 0) {
                    new_in.insert(var);
                }
            }
            
            if (new_in != result.in_sets[block.id] || new_out != result.out_sets[block.id]) {
                result.in_sets[block.id] = new_in;
                result.out_sets[block.id] = new_out;
                changed = true;
            }
        }
    }
    
    return result;
}

DataFlowAnalysis::TransferFunction LiveVariableAnalysis::getTransferFunction() {
    return [](const Domain& in, const IRInstruction& instr) -> Domain {
        Domain out = in;
        
        // Remove variable if it's defined
        if (instr.result.isVariable()) {
            int var_id = instr.result.asInt();
            out.erase(var_id);
        }
        
        // Add variable if it's used
        if (instr.operand1.isVariable()) {
            out.insert(instr.operand1.asInt());
        }
        
        if (instr.operand2.isVariable()) {
            out.insert(instr.operand2.asInt());
        }
        
        return out;
    };
}

// DeadCodeEliminationPass implementation
OptimizationResult DeadCodeEliminationPass::run(std::vector<IRInstruction>& ir_code, 
                                                  OptimizationContext& context) {
    OptimizationResult result;
    
    // Build control flow graph
    ControlFlowGraph cfg;
    cfg.buildFromIR(ir_code);
    context.setControlFlowGraph(cfg);
    
    // Perform live variable analysis
    LiveVariableAnalysis lva;
    auto analysis_result = lva.analyze(cfg);
    context.setAnalysisResult("live_variables", analysis_result);
    
    // Find live variables at each instruction
    std::unordered_set<int> live_vars;
    for (auto it = ir_code.rbegin(); it != ir_code.rend(); ++it) {
        const auto& instr = *it;
        
        // Check if instruction result is live
        if (instr.result.isVariable()) {
            int var_id = instr.result.asInt();
            if (live_vars.count(var_id) == 0) {
                // Instruction is dead
                result.instructions_removed++;
                continue;
            }
        }
        
        // Update live variables
        if (instr.operand1.isVariable()) {
            live_vars.insert(instr.operand1.asInt());
        }
        
        if (instr.operand2.isVariable()) {
            live_vars.insert(instr.operand2.asInt());
        }
        
        if (instr.result.isVariable()) {
            live_vars.erase(instr.result.asInt());
        }
    }
    
    result.success = true;
    return result;
}

// ConstantFoldingPass implementation
OptimizationResult ConstantFoldingPass::run(std::vector<IRInstruction>& ir_code, 
                                             OptimizationContext& context) {
    OptimizationResult result;
    
    for (auto& instr : ir_code) {
        IRInstruction folded = foldInstruction(instr);
        if (folded.opcode != instr.opcode) {
            instr = folded;
            result.instructions_removed++;
        }
    }
    
    result.success = true;
    return result;
}

bool ConstantFoldingPass::isConstant(const Value& val) {
    return val.isConstant();
}

IRInstruction ConstantFoldingPass::foldInstruction(const IRInstruction& instr) {
    if (!isConstant(instr.operand1) || !isConstant(instr.operand2)) {
        return instr;
    }
    
    Value result = evaluateConstantExpression(instr);
    if (result.isValid()) {
        return IRInstruction(IROpcode::LOAD_CONST, result, instr.result);
    }
    
    return instr;
}

Value ConstantFoldingPass::evaluateConstantExpression(const IRInstruction& instr) {
    if (!isConstant(instr.operand1) || !isConstant(instr.operand2)) {
        return Value{};
    }
    
    switch (instr.opcode) {
        case IROpcode::ADD: {
            if (instr.operand1.isInt() && instr.operand2.isInt()) {
                return Value(instr.operand1.asInt() + instr.operand2.asInt());
            } else if (instr.operand1.isFloat() && instr.operand2.isFloat()) {
                return Value(instr.operand1.asFloat() + instr.operand2.asFloat());
            }
            break;
        }
        case IROpcode::SUBTRACT: {
            if (instr.operand1.isInt() && instr.operand2.isInt()) {
                return Value(instr.operand1.asInt() - instr.operand2.asInt());
            } else if (instr.operand1.isFloat() && instr.operand2.isFloat()) {
                return Value(instr.operand1.asFloat() - instr.operand2.asFloat());
            }
            break;
        }
        case IROpcode::MULTIPLY: {
            if (instr.operand1.isInt() && instr.operand2.isInt()) {
                return Value(instr.operand1.asInt() * instr.operand2.asInt());
            } else if (instr.operand1.isFloat() && instr.operand2.isFloat()) {
                return Value(instr.operand1.asFloat() * instr.operand2.asFloat());
            }
            break;
        }
        case IROpcode::DIVIDE: {
            if (instr.operand1.isInt() && instr.operand2.isInt()) {
                if (instr.operand2.asInt() != 0) {
                    return Value(instr.operand1.asInt() / instr.operand2.asInt());
                }
            } else if (instr.operand1.isFloat() && instr.operand2.isFloat()) {
                if (instr.operand2.asFloat() != 0.0) {
                    return Value(instr.operand1.asFloat() / instr.operand2.asFloat());
                }
            }
            break;
        }
        default:
            break;
    }
    
    return Value{};
}

// CommonSubexpressionEliminationPass implementation
OptimizationResult CommonSubexpressionEliminationPass::run(std::vector<IRInstruction>& ir_code, 
                                                           OptimizationContext& context) {
    OptimizationResult result;
    expression_map.clear();
    
    for (auto& instr : ir_code) {
        std::string key = getExpressionKey(instr);
        
        if (!key.empty()) {
            auto it = expression_map.find(key);
            if (it != expression_map.end()) {
                // Replace with previous result
                instr = IRInstruction(IROpcode::MOVE, Value(it->second), instr.result);
                result.instructions_removed++;
            } else {
                // Store expression
                if (instr.result.isVariable()) {
                    expression_map[key] = instr.result.asInt();
                }
            }
        }
    }
    
    result.success = true;
    return result;
}

std::string CommonSubexpressionEliminationPass::getExpressionKey(const IRInstruction& instr) {
    if (instr.opcode == IROpcode::ADD || instr.opcode == IROpcode::MULTIPLY ||
        instr.opcode == IROpcode::SUBTRACT || instr.opcode == IROpcode::DIVIDE) {
        
        std::ostringstream oss;
        oss << static_cast<int>(instr.opcode) << ":";
        oss << instr.operand1.toString() << ":";
        oss << instr.operand2.toString();
        
        return oss.str();
    }
    
    return "";
}

// OptimizationContext implementation
void OptimizationContext::addPass(std::unique_ptr<OptimizationPass> pass) {
    passes.push_back(std::move(pass));
}

void OptimizationContext::setupDefaultPasses() {
    passes.clear();
    
    // Add passes in order
    addPass(std::make_unique<ConstantFoldingPass>());
    addPass(std::make_unique<DeadCodeEliminationPass>());
    addPass(std::make_unique<CommonSubexpressionEliminationPass>());
    addPass(std::make_unique<StrengthReductionPass>());
    addPass(std::make_unique<LoopInvariantCodeMotionPass>());
    addPass(std::make_unique<TailCallOptimizationPass>());
    addPass(std::make_unique<InliningOptimizer>());
    addPass(std::make_unique<VectorizationOptimizer>());
    addPass(std::make_unique<EscapeAnalysisOptimizer>());
    addPass(std::make_unique<PeepholeOptimizationPass>());
}

OptimizationResult OptimizationContext::runOptimizations(std::vector<IRInstruction>& ir_code) {
    OptimizationResult total_result;
    
    // Build control flow graph
    ControlFlowGraph cfg;
    cfg.buildFromIR(ir_code);
    setControlFlowGraph(cfg);
    
    // Run passes
    for (auto& pass : passes) {
        if (pass->shouldRun(level)) {
            auto pass_result = pass->run(ir_code, *this);
            
            if (pass_result.success) {
                total_result.instructions_removed += pass_result.instructions_removed;
                total_result.instructions_added += pass_result.instructions_added;
                total_result.loops_optimized += pass_result.loops_optimized;
                total_result.functions_inlined += pass_result.functions_inlined;
                total_result.speedup_factor *= pass_result.speedup_factor;
                
                // Rebuild CFG after significant changes
                if (pass_result.instructions_removed > 0) {
                    cfg.buildFromIR(ir_code);
                    setControlFlowGraph(cfg);
                }
            }
            
            // Add warnings and errors
            total_result.warnings.insert(total_result.warnings.end(),
                                      pass_result.warnings.begin(),
                                      pass_result.warnings.end());
            total_result.errors.insert(total_result.errors.end(),
                                    pass_result.errors.begin(),
                                    pass_result.errors.end());
        }
    }
    
    total_result.success = total_result.errors.empty();
    return total_result;
}

// OptimizationManager implementation
OptimizationManager::OptimizationManager(OptimizationLevel level, bool verbose) {
    context = std::make_unique<OptimizationContext>(level, verbose);
    context->setupDefaultPasses();
}

void OptimizationManager::optimizeFunction(const std::string& name, std::vector<IRInstruction>& ir_code) {
    auto result = context->runOptimizations(ir_code);
    results[name] = result;
}

void OptimizationManager::optimizeModule(std::unordered_map<std::string, std::vector<IRInstruction>>& functions) {
    for (auto& [name, ir_code] : functions) {
        optimizeFunction(name, ir_code);
    }
}

void OptimizationManager::addCustomPass(std::unique_ptr<OptimizationPass> pass) {
    context->addPass(std::move(pass));
}

void OptimizationManager::setOptimizationLevel(OptimizationLevel level) {
    context->setLevel(level);
}

OptimizationResult OptimizationManager::getFunctionResult(const std::string& name) const {
    auto it = results.find(name);
    return it != results.end() ? it->second : OptimizationResult{};
}

std::unordered_map<std::string, OptimizationResult> OptimizationManager::getAllResults() const {
    return results;
}

void OptimizationManager::printOptimizationReport() const {
    std::cout << "Optimization Report:\n";
    std::cout << "==================\n\n";
    
    for (const auto& [name, result] : results) {
        std::cout << "Function: " << name << "\n";
        std::cout << "  Success: " << (result.success ? "Yes" : "No") << "\n";
        std::cout << "  Instructions removed: " << result.instructions_removed << "\n";
        std::cout << "  Instructions added: " << result.instructions_added << "\n";
        std::cout << "  Loops optimized: " << result.loops_optimized << "\n";
        std::cout << "  Functions inlined: " << result.functions_inlined << "\n";
        std::cout << "  Speedup factor: " << result.speedup_factor << "x\n";
        
        if (!result.warnings.empty()) {
            std::cout << "  Warnings:\n";
            for (const auto& warning : result.warnings) {
                std::cout << "    " << warning << "\n";
            }
        }
        
        if (!result.errors.empty()) {
            std::cout << "  Errors:\n";
            for (const auto& error : result.errors) {
                std::cout << "    " << error << "\n";
            }
        }
        
        std::cout << "\n";
    }
}

void OptimizationManager::reset() {
    results.clear();
}

// OptimizationProfiler implementation
void OptimizationProfiler::startPass(const std::string& pass_name) {
    if (!enabled) return;
    
    start_times[pass_name] = std::chrono::high_resolution_clock::now();
}

void OptimizationProfiler::endPass(const std::string& pass_name, const OptimizationResult& result) {
    if (!enabled) return;
    
    auto it = start_times.find(pass_name);
    if (it == start_times.end()) return;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - it->second);
    
    PassProfile& profile = profiles[pass_name];
    profile.pass_name = pass_name;
    profile.execution_count++;
    profile.total_time_ns += duration.count();
    profile.total_instructions_removed += result.instructions_removed;
    profile.total_instructions_added += result.instructions_added;
    profile.total_speedup += result.speedup_factor;
    
    start_times.erase(it);
}

OptimizationProfiler::PassProfile OptimizationProfiler::getProfile(const std::string& pass_name) const {
    auto it = profiles.find(pass_name);
    return it != profiles.end() ? it->second : PassProfile{};
}

std::unordered_map<std::string, OptimizationProfiler::PassProfile> OptimizationProfiler::getAllProfiles() const {
    return profiles;
}

void OptimizationProfiler::printProfileReport() const {
    std::cout << "Optimization Profile Report:\n";
    std::cout << "==========================\n\n";
    
    for (const auto& [name, profile] : profiles) {
        std::cout << "Pass: " << name << "\n";
        std::cout << "  Executions: " << profile.execution_count << "\n";
        std::cout << "  Total time: " << (profile.total_time_ns / 1000000.0) << " ms\n";
        std::cout << "  Avg time: " << (profile.total_time_ns / profile.execution_count / 1000000.0) << " ms\n";
        std::cout << "  Total instructions removed: " << profile.total_instructions_removed << "\n";
        std::cout << "  Total instructions added: " << profile.total_instructions_added << "\n";
        std::cout << "  Total speedup: " << profile.total_speedup << "x\n";
        std::cout << "  Avg speedup: " << (profile.total_speedup / profile.execution_count) << "x\n";
        std::cout << "\n";
    }
}

void OptimizationProfiler::reset() {
    profiles.clear();
    start_times.clear();
}

} // namespace pyplusplus