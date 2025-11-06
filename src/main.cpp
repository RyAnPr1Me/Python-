#include "lexer.h"
#include "parser.h"
#include "type_inference.h"
#include "ir.h"
#include "codegen.h"
#include "runtime.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <getopt.h>

namespace pyplusplus {

class Compiler {
private:
    TypeSystem type_system;
    std::string input_file;
    std::string output_file;
    int optimization_level = 2;
    bool debug_mode = false;
    bool verbose = false;
    bool emit_llvm = false;
    bool emit_ir = false;
    
    // Compilation pipeline
    std::string readSourceFile(const std::string& filename);
    std::unique_ptr<Module> parseSource(const std::string& source);
    bool performTypeInference(Module& module);
    std::unique_ptr<IRModule> generateIR(Module& module);
    bool generateCode(IRModule& ir_module);
    bool linkExecutable();
    
    // Error reporting
    void reportError(const std::string& error);
    void reportWarning(const std::string& warning);
    
public:
    Compiler() {
        Runtime::initialize();
    }
    
    ~Compiler() {
        Runtime::shutdown();
    }
    
    bool compile(int argc, char* argv[]);
    
    // Configuration
    void setInputFile(const std::string& file) { input_file = file; }
    void setOutputFile(const std::string& file) { output_file = file; }
    void setOptimizationLevel(int level) { optimization_level = level; }
    void setDebugMode(bool debug) { debug_mode = debug; }
    void setVerbose(bool verbose) { this->verbose = verbose; }
    void setEmitLLVM(bool emit) { emit_llvm = emit; }
    void setEmitIR(bool emit) { emit_ir = emit; }
};

std::string Compiler::readSourceFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        reportError("Cannot open input file: " + filename);
        return "";
    }
    
    std::string source((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    return source;
}

std::unique_ptr<Module> Compiler::parseSource(const std::string& source) {
    if (verbose) {
        std::cout << "Parsing source code..." << std::endl;
    }
    
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        
        Parser parser(tokens);
        auto module = parser.parse();
        
        if (verbose) {
            std::cout << "Parsing completed successfully" << std::endl;
        }
        
        return module;
    } catch (const std::exception& e) {
        reportError("Parse error: " + std::string(e.what()));
        return nullptr;
    }
}

bool Compiler::performTypeInference(Module& module) {
    if (verbose) {
        std::cout << "Performing type inference..." << std::endl;
    }
    
    try {
        TypeInference inference(type_system);
        bool success = inference.infer(module);
        
        if (!success) {
            reportError("Type inference failed");
            return false;
        }
        
        // Type checking
        TypeChecker checker(type_system, inference);
        success = checker.check(module);
        
        if (!success) {
            const auto& errors = checker.getErrors();
            for (const auto& error : errors) {
                reportError(error);
            }
            return false;
        }
        
        if (verbose) {
            std::cout << "Type inference completed successfully" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        reportError("Type inference error: " + std::string(e.what()));
        return false;
    }
}

std::unique_ptr<IRModule> Compiler::generateIR(Module& module) {
    if (verbose) {
        std::cout << "Generating intermediate representation..." << std::endl;
    }
    
    try {
        // Create IR module
        auto ir_module = std::make_unique<IRModule>("main");
        
        // Generate IR for each function
        // This is a simplified IR generation - in practice, we'd have a proper IRBuilder
        for (const auto& stmt : module.getStatements()) {
            if (auto func_def = dynamic_cast<FunctionDef*>(stmt.get())) {
                // Create function type
                std::vector<std::shared_ptr<Type>> param_types;
                for (const auto& [name, type_annotation] : func_def->getParameters()) {
                    // For now, use dynamic types
                    param_types.push_back(type_system.getAnyType());
                }
                
                auto return_type = type_system.getAnyType();
                auto func_type = type_system.createFunctionType(std::move(param_types), return_type);
                
                // Create IR function
                auto ir_func = ir_module->createFunction(func_def->getName(), func_type);
                
                // Create entry block
                auto entry_block = ir_func->createBasicBlock("entry");
                
                // Add a simple return statement for now
                // In practice, we'd generate proper IR for the function body
                IRBuilder builder(ir_func.get(), entry_block.get());
                auto none_value = builder.createConstant(nullptr, type_system.getNoneType());
                builder.createReturn(none_value);
            }
        }
        
        if (verbose) {
            std::cout << "IR generation completed successfully" << std::endl;
        }
        
        if (emit_ir) {
            std::string ir_filename = output_file + ".ir";
            std::ofstream ir_file(ir_filename);
            if (ir_file.is_open()) {
                ir_file << ir_module->toString();
                ir_file.close();
                std::cout << "IR written to: " << ir_filename << std::endl;
            }
        }
        
        return ir_module;
    } catch (const std::exception& e) {
        reportError("IR generation error: " + std::string(e.what()));
        return nullptr;
    }
}

bool Compiler::generateCode(IRModule& ir_module) {
    if (verbose) {
        std::cout << "Generating LLVM code..." << std::endl;
    }
    
    try {
        LLVMCodeGenerator code_gen(type_system);
        
        if (!code_gen.generate(ir_module)) {
            reportError("LLVM code generation failed");
            return false;
        }
        
        // Optimize
        if (optimization_level > 0) {
            if (verbose) {
                std::cout << "Optimizing code (level " << optimization_level << ")..." << std::endl;
            }
            
            if (!code_gen.optimize(optimization_level)) {
                reportWarning("Optimization failed, continuing with unoptimized code");
            }
        }
        
        // Emit LLVM IR if requested
        if (emit_llvm) {
            std::string llvm_filename = output_file + ".ll";
            if (!code_gen.writeToFile(llvm_filename)) {
                reportError("Failed to write LLVM IR file");
                return false;
            }
            std::cout << "LLVM IR written to: " << llvm_filename << std::endl;
        }
        
        // Generate object file
        std::string obj_filename = output_file + ".o";
        if (!code_gen.writeObjectFile(obj_filename)) {
            reportError("Failed to write object file");
            return false;
        }
        
        if (verbose) {
            std::cout << "Object file written to: " << obj_filename << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        reportError("Code generation error: " + std::string(e.what()));
        return false;
    }
}

bool Compiler::linkExecutable() {
    if (verbose) {
        std::cout << "Linking executable..." << std::endl;
    }
    
    // For simplicity, we'll use a basic system call to link
    // In a real implementation, we'd use LLVM's linking capabilities
    std::string obj_filename = output_file + ".o";
    std::string link_command = "clang++ -o " + output_file + " " + obj_filename + " -lpy++-runtime";
    
    int result = std::system(link_command.c_str());
    if (result != 0) {
        reportError("Linking failed with exit code: " + std::to_string(result));
        return false;
    }
    
    if (verbose) {
        std::cout << "Executable created: " << output_file << std::endl;
    }
    
    return true;
}

void Compiler::reportError(const std::string& error) {
    std::cerr << "Error: " << error << std::endl;
}

void Compiler::reportWarning(const std::string& warning) {
    std::cerr << "Warning: " << warning << std::endl;
}

bool Compiler::compile(int argc, char* argv[]) {
    // Parse command line arguments
    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"optimize", required_argument, 0, 'O'},
        {"debug", no_argument, 0, 'g'},
        {"verbose", no_argument, 0, 'v'},
        {"emit-llvm", no_argument, 0, 'S'},
        {"emit-ir", no_argument, 0, 'I'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "o:O:gvSIh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'o':
                output_file = optarg;
                break;
            case 'O':
                optimization_level = std::stoi(optarg);
                if (optimization_level < 0 || optimization_level > 3) {
                    reportError("Optimization level must be between 0 and 3");
                    return false;
                }
                break;
            case 'g':
                debug_mode = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'S':
                emit_llvm = true;
                break;
            case 'I':
                emit_ir = true;
                break;
            case 'h':
                std::cout << "Python++ Compiler\n"
                          << "Usage: py++c [options] <input_file>\n\n"
                          << "Options:\n"
                          << "  -o <file>     Output file name\n"
                          << "  -O <level>    Optimization level (0-3)\n"
                          << "  -g            Enable debug mode\n"
                          << "  -v            Verbose output\n"
                          << "  -S            Emit LLVM IR\n"
                          << "  -I            Emit intermediate representation\n"
                          << "  -h            Show this help message\n";
                return true;
            case '?':
                return false;
            default:
                break;
        }
    }
    
    if (optind >= argc) {
        reportError("No input file specified");
        return false;
    }
    
    input_file = argv[optind];
    
    if (output_file.empty()) {
        // Generate default output filename
        size_t dot_pos = input_file.find_last_of('.');
        if (dot_pos != std::string::npos) {
            output_file = input_file.substr(0, dot_pos);
        } else {
            output_file = input_file;
        }
    }
    
    if (verbose) {
        std::cout << "Compiling: " << input_file << " -> " << output_file << std::endl;
        std::cout << "Optimization level: " << optimization_level << std::endl;
        std::cout << "Debug mode: " << (debug_mode ? "enabled" : "disabled") << std::endl;
    }
    
    // Compilation pipeline
    std::string source = readSourceFile(input_file);
    if (source.empty()) {
        return false;
    }
    
    auto module = parseSource(source);
    if (!module) {
        return false;
    }
    
    if (!performTypeInference(*module)) {
        return false;
    }
    
    auto ir_module = generateIR(*module);
    if (!ir_module) {
        return false;
    }
    
    if (!generateCode(*ir_module)) {
        return false;
    }
    
    if (!linkExecutable()) {
        return false;
    }
    
    if (verbose) {
        std::cout << "Compilation completed successfully!" << std::endl;
    }
    
    return true;
}

} // namespace pyplusplus

int main(int argc, char* argv[]) {
    try {
        pyplusplus::Compiler compiler;
        return compiler.compile(argc, argv) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}