#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#define PATH_SEPARATOR "/"
#endif

namespace fs = std::filesystem;

// Get the directory of the current executable
std::string getExecutableDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return fullPath.substr(0, pos);
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("/");
        return fullPath.substr(0, pos);
    }
    return ".";
#endif
}

// Check if file has .py+ or .py extension
bool isPythonPlusPlusFile(const std::string& filename) {
    std::string ext;
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        ext = filename.substr(dotPos);
    }
    return ext == ".py+" || ext == ".py";
}

// Compile and run a Python++ source file
int compileAndRun(const std::string& sourceFile, const std::vector<std::string>& args) {
    std::string execDir = getExecutableDir();
    std::string compiler = execDir + PATH_SEPARATOR + "py++c";
#ifdef _WIN32
    compiler += ".exe";
#endif

    // Check if compiler exists
    if (!fs::exists(compiler)) {
        std::cerr << "Error: py++c compiler not found at: " << compiler << std::endl;
        return 1;
    }

    // Create temporary output file
    std::string tempOutput;
#ifdef _WIN32
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    tempOutput = std::string(tempPath) + "py++_temp_" + std::to_string(GetCurrentProcessId()) + ".exe";
#else
    tempOutput = "/tmp/py++_temp_" + std::to_string(getpid());
#endif

    // Build compile command
    std::string compileCmd = "\"" + compiler + "\" \"" + sourceFile + "\" -o \"" + tempOutput + "\"";
    
    // Compile the source file
    int compileResult = system(compileCmd.c_str());
    if (compileResult != 0) {
        std::cerr << "Error: Compilation failed" << std::endl;
        return compileResult;
    }

    // Build run command
    std::string runCmd = "\"" + tempOutput + "\"";
    for (const auto& arg : args) {
        runCmd += " \"" + arg + "\"";
    }

    // Run the compiled program
    int runResult = system(runCmd.c_str());

    // Clean up temporary file
    try {
        fs::remove(tempOutput);
    } catch (...) {
        // Ignore cleanup errors
    }

    return runResult;
}

void printUsage() {
    std::cout << "Python++ Runner (p++)\n";
    std::cout << "Usage: p++ <script.py+> [arguments...]\n";
    std::cout << "       p++ <script.py> [arguments...]\n";
    std::cout << "\n";
    std::cout << "Compiles and runs a Python++ source file.\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help     Show this help message\n";
    std::cout << "  -v, --version  Show version information\n";
}

void printVersion() {
    std::cout << "Python++ Runner v1.0.0\n";
    std::cout << "An AOT-compiled Python language with 100% Python syntax compatibility\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string firstArg = argv[1];
    
    if (firstArg == "-h" || firstArg == "--help") {
        printUsage();
        return 0;
    }
    
    if (firstArg == "-v" || firstArg == "--version") {
        printVersion();
        return 0;
    }

    // Check if the source file exists
    std::string sourceFile = firstArg;
    if (!fs::exists(sourceFile)) {
        std::cerr << "Error: File not found: " << sourceFile << std::endl;
        return 1;
    }

    // Check if it's a valid Python++ file
    if (!isPythonPlusPlusFile(sourceFile)) {
        std::cerr << "Warning: File doesn't have .py+ or .py extension: " << sourceFile << std::endl;
    }

    // Collect remaining arguments to pass to the program
    std::vector<std::string> programArgs;
    for (int i = 2; i < argc; i++) {
        programArgs.push_back(argv[i]);
    }

    return compileAndRun(sourceFile, programArgs);
}
