#pragma once
#include "ir.h"
#include "runtime.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>

namespace pyplusplus {

// Forward declarations
class AOTCompiler;
class CodeGenerator;
class Linker;
class ObjectFile;

// AOT compilation targets
enum class CompilationTarget {
    NATIVE_EXECUTABLE,  // Native executable
    SHARED_LIBRARY,     // Shared library (.so/.dll/.dylib)
    OBJECT_FILE,        // Object file (.o/.obj)
    LLVM_IR,           // LLVM IR file (.ll)
    LLVM_BITCODE,      // LLVM bitcode (.bc)
    ASSEMBLY           // Assembly file (.s)
};

// Compilation architectures
enum class Architecture {
    X86_64,
    ARM64,
    RISCV64,
    WASM32,
    WASM64
};

// Optimization levels for AOT
enum class AOTOptimizationLevel {
    O0,   // No optimization
    O1,   // Basic optimizations
    O2,   // Standard optimizations
    O3,   // Aggressive optimizations
    Os,   // Optimize for size
    Oz    // Optimize for size aggressively
};

// AOT compilation configuration
struct AOTConfig {
    CompilationTarget target = CompilationTarget::NATIVE_EXECUTABLE;
    Architecture arch = Architecture::X86_64;
    AOTOptimizationLevel opt_level = AOTOptimizationLevel::O2;
    bool enable_lto = true;           // Link-time optimization
    bool enable_debug_info = false;   // Debug symbols
    bool enable_assertions = false;    // Runtime assertions
    bool strip_binary = true;         // Strip debug info from final binary
    std::string output_file = "output";
    std::vector<std::string> link_libraries;
    std::vector<std::string> include_paths;
    std::vector<std::string> library_paths;
    std::string target_triple;        // LLVM target triple
};

// Compiled object file representation
class ObjectFile {
private:
    std::string filename;
    std::vector<uint8_t> data;
    std::vector<std::string> symbols;
    std::vector<std::string> undefined_symbols;
    
public:
    explicit ObjectFile(const std::string& filename) : filename(filename) {}
    
    const std::string& getFilename() const { return filename; }
    const std::vector<uint8_t>& getData() const { return data; }
    const std::vector<std::string>& getSymbols() const { return symbols; }
    const std::vector<std::string>& getUndefinedSymbols() const { return undefined_symbols; }
    
    void setData(const std::vector<uint8_t>& new_data) { data = new_data; }
    void addSymbol(const std::string& symbol) { symbols.push_back(symbol); }
    void addUndefinedSymbol(const std::string& symbol) { undefined_symbols.push_back(symbol); }
    
    bool writeToFile() const;
    bool readFromFile();
};

// AOT code generator
class CodeGenerator {
private:
    AOTConfig config;
    void* llvm_context;
    void* llvm_module;
    void* llvm_builder;
    void* llvm_pass_manager;
    
public:
    explicit CodeGenerator(const AOTConfig& config);
    ~CodeGenerator();
    
    // Code generation methods
    bool generateCode(const std::vector<IRInstruction>& ir_code, const std::string& function_name);
    bool generateModule(const std::vector<std::pair<std::string, std::vector<IRInstruction>>>& functions);
    
    // Optimization passes
    void optimizeModule();
    void applyOptimizations();
    
    // Output generation
    std::unique_ptr<ObjectFile> generateObjectFile(const std::string& filename);
    bool generateLLVMIR(const std::string& filename);
    bool generateBitcode(const std::string& filename);
    bool generateAssembly(const std::string& filename);
    
    // Configuration
    void setConfig(const AOTConfig& new_config) { config = new_config; }
    const AOTConfig& getConfig() const { return config; }
    
private:
    void initializeLLVM();
    void shutdownLLVM();
    void setupTargetTriple();
    void setupPassManager();
    
    // LLVM IR generation
    void* generateFunctionIR(const std::vector<IRInstruction>& ir_code, const std::string& name);
    void* generateTypeIR(const Type* type);
    void* generateValueIR(const Value& value);
    
    // Optimization helpers
    void setupOptimizationPasses();
    void runFunctionPasses();
    void runModulePasses();
};

// Linker for AOT compilation
class Linker {
private:
    AOTConfig config;
    std::vector<std::unique_ptr<ObjectFile>> object_files;
    
public:
    explicit Linker(const AOTConfig& config) : config(config) {}
    
    // Linking operations
    bool addObjectFile(std::unique_ptr<ObjectFile> obj_file);
    bool linkExecutable(const std::string& output_path);
    bool linkSharedLibrary(const std::string& output_path);
    
    // Symbol resolution
    std::vector<std::string> getUndefinedSymbols() const;
    std::vector<std::string> getDuplicateSymbols() const;
    bool resolveSymbols();
    
    // Configuration
    void addLibrary(const std::string& library) { config.link_libraries.push_back(library); }
    void addLibraryPath(const std::string& path) { config.library_paths.push_back(path); }
    
private:
    bool invokeSystemLinker(const std::string& output_path, const std::vector<std::string>& args);
    std::vector<std::string> buildLinkerArgs(const std::string& output_path);
};

// Main AOT compiler
class AOTCompiler {
private:
    AOTConfig config;
    std::unique_ptr<CodeGenerator> code_generator;
    std::unique_ptr<Linker> linker;
    std::unordered_map<std::string, std::vector<IRInstruction>> compiled_functions;
    
public:
    explicit AOTCompiler(const AOTConfig& config = AOTConfig{});
    ~AOTCompiler();
    
    // Compilation interface
    bool compileFunction(const std::string& name, const std::vector<IRInstruction>& ir_code);
    bool compileModule(const std::vector<std::pair<std::string, std::vector<IRInstruction>>>& functions);
    
    // Output generation
    bool generateExecutable(const std::string& output_path);
    bool generateSharedLibrary(const std::string& output_path);
    bool generateObjectFile(const std::string& output_path);
    bool generateLLVMIR(const std::string& output_path);
    bool generateAssembly(const std::string& output_path);
    
    // Configuration
    void setConfig(const AOTConfig& new_config);
    const AOTConfig& getConfig() const { return config; }
    
    // Analysis and optimization
    void analyzeCode();
    void optimizeCode();
    std::vector<std::string> getCompilationWarnings() const;
    std::vector<std::string> getCompilationErrors() const;
    
    // Statistics
    struct CompilationStats {
        size_t functions_compiled = 0;
        size_t lines_of_code = 0;
        size_t optimization_passes_run = 0;
        size_t compilation_time_ms = 0;
        size_t binary_size_bytes = 0;
    };
    
    CompilationStats getStats() const;
    
private:
    bool validateIR(const std::vector<IRInstruction>& ir_code);
    bool optimizeFunction(const std::string& name, std::vector<IRInstruction>& ir_code);
    bool linkOutput(const std::string& output_path);
    
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    CompilationStats stats;
};

// AOT compilation pipeline
class AOTPipeline {
private:
    std::unique_ptr<AOTCompiler> compiler;
    AOTConfig config;
    
public:
    explicit AOTPipeline(const AOTConfig& config = AOTConfig{});
    
    // Pipeline stages
    bool parseSource(const std::string& source_file);
    bool typeCheck();
    bool generateIR();
    bool optimizeIR();
    bool generateCode();
    bool linkBinary();
    
    // Full compilation
    bool compileToExecutable(const std::string& source_file, const std::string& output_path);
    bool compileToLibrary(const std::string& source_file, const std::string& output_path);
    
    // Batch compilation
    bool compileProject(const std::vector<std::string>& source_files, const std::string& output_path);
    
    // Configuration
    void setConfig(const AOTConfig& new_config);
    const AOTConfig& getConfig() const { return config; }
    
    // Progress reporting
    void setProgressCallback(std::function<void(const std::string&)> callback);
    
private:
    std::function<void(const std::string&)> progress_callback;
    
    void reportProgress(const std::string& stage);
};

// Cross-compilation support
class CrossCompiler {
private:
    Architecture target_arch;
    std::string target_triple;
    std::string sysroot;
    
public:
    CrossCompiler(Architecture arch, const std::string& triple, const std::string& sysroot = "")
        : target_arch(arch), target_triple(triple), sysroot(sysroot) {}
    
    bool setupCrossCompilationEnvironment();
    std::string getTargetTriple() const { return target_triple; }
    Architecture getTargetArchitecture() const { return target_arch; }
    
    // Toolchain management
    bool installToolchain();
    std::string getCompilerPath() const;
    std::string getLinkerPath() const;
    std::vector<std::string> getCompilerFlags() const;
    
private:
    bool validateToolchain();
    std::string detectSystemTriple();
};

// Build system integration
class BuildSystem {
private:
    std::vector<std::string> source_files;
    std::vector<std::string> include_dirs;
    std::vector<std::string> library_dirs;
    std::vector<std::string> libraries;
    AOTConfig config;
    
public:
    BuildSystem() = default;
    
    // Project configuration
    void addSourceFile(const std::string& file) { source_files.push_back(file); }
    void addIncludeDirectory(const std::string& dir) { include_dirs.push_back(dir); }
    void addLibraryDirectory(const std::string& dir) { library_dirs.push_back(dir); }
    void addLibrary(const std::string& lib) { libraries.push_back(lib); }
    
    // Build operations
    bool buildExecutable(const std::string& output_path);
    bool buildLibrary(const std::string& output_path);
    bool buildProject(const std::string& build_dir = "build");
    
    // Dependency management
    bool generateDependencies();
    std::vector<std::string> getDependencies() const;
    
    // Configuration
    void setConfig(const AOTConfig& new_config) { config = new_config; }
    const AOTConfig& getConfig() const { return config; }
    
private:
    bool createBuildDirectory(const std::string& build_dir);
    std::vector<std::string> getChangedFiles() const;
    bool needsRebuild() const;
};

// Utility functions
namespace AOTUtils {
    // Target detection
    Architecture detectHostArchitecture();
    std::string getDefaultTargetTriple();
    std::vector<std::string> getSupportedTargets();
    
    // Configuration helpers
    AOTConfig createDefaultConfig();
    AOTConfig createDebugConfig();
    AOTConfig createReleaseConfig();
    AOTConfig createSizeOptimizedConfig();
    
    // File operations
    bool ensureDirectoryExists(const std::string& path);
    std::string getFileExtension(const std::string& filename);
    std::string changeFileExtension(const std::string& filename, const std::string& new_ext);
    
    // Compilation helpers
    bool validateSourceFile(const std::string& filename);
    std::vector<std::string> findSourceFiles(const std::string& directory);
    std::string generateOutputFilename(const std::string& input, CompilationTarget target);
}

} // namespace pyplusplus