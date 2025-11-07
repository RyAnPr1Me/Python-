#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define PATH_SEPARATOR "\\"
#else
#include <unistd.h>
#define PATH_SEPARATOR "/"
#endif

namespace fs = std::filesystem;

// Escape special characters for shell commands
std::string escapeShellArg(const std::string& arg) {
#ifdef _WIN32
    // Windows: escape quotes and backslashes
    std::string escaped;
    for (char c : arg) {
        if (c == '"' || c == '\\') {
            escaped += '\\';
        }
        escaped += c;
    }
    return "\"" + escaped + "\"";
#else
    // Unix: use single quotes and escape single quotes within
    std::string escaped;
    for (char c : arg) {
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    return "'" + escaped + "'";
#endif
}

// Get the directory of the current executable
std::string getExecutableDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD result = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (result == 0 || result == MAX_PATH) {
        std::cerr << "Error: Failed to get executable path" << std::endl;
        return ".";
    }
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    return fullPath.substr(0, pos);
#else
    char path[4096];  // Increased buffer size
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        std::cerr << "Warning: Failed to read executable path, using current directory" << std::endl;
        return ".";
    }
    if (len >= static_cast<ssize_t>(sizeof(path) - 1)) {
        std::cerr << "Warning: Executable path may be truncated" << std::endl;
    }
    path[len] = '\0';
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("/");
    return fullPath.substr(0, pos);
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

    // Build compile command with proper escaping
    std::string compileCmd = escapeShellArg(compiler) + " " + escapeShellArg(sourceFile) + " -o " + escapeShellArg(tempOutput);
    
    // Compile the source file
    int compileResult = system(compileCmd.c_str());
    if (compileResult != 0) {
        std::cerr << "Error: Compilation failed" << std::endl;
        return compileResult;
    }

    // Build run command with proper escaping
    std::string runCmd = escapeShellArg(tempOutput);
    for (const auto& arg : args) {
        runCmd += " " + escapeShellArg(arg);
    }

    // Run the compiled program
    int runResult = system(runCmd.c_str());

    // Clean up temporary file
    try {
        if (fs::exists(tempOutput)) {
            fs::remove(tempOutput);
        }
    } catch (const fs::filesystem_error& e) {
        // Log but don't fail on cleanup errors
        std::cerr << "Warning: Failed to remove temporary file: " << e.what() << std::endl;
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
    std::cout << "  -h, --help        Show this help message\n";
    std::cout << "  -v, --version     Show version information\n";
    std::cout << "  --install         Install file associations for .py+ files (Windows only, requires admin)\n";
    std::cout << "  --uninstall       Remove file associations for .py+ files (Windows only, requires admin)\n";
}

void printVersion() {
    std::cout << "Python++ Runner v1.0.0\n";
    std::cout << "An AOT-compiled Python language with 100% Python syntax compatibility\n";
}

#ifdef _WIN32
// Check if running with administrator privileges on Windows
bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin == TRUE;
}

// Get the full path to the current executable
std::string getExecutablePath() {
    char path[MAX_PATH];
    DWORD result = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (result == 0 || result == MAX_PATH) {
        return "";
    }
    return std::string(path);
}

// Install file associations on Windows
int installFileAssociations() {
    if (!isRunningAsAdmin()) {
        std::cerr << "Error: Administrator privileges required for installation.\n";
        std::cerr << "Please run this command from an Administrator command prompt.\n";
        return 1;
    }
    
    std::string exePath = getExecutablePath();
    if (exePath.empty()) {
        std::cerr << "Error: Failed to get executable path\n";
        return 1;
    }
    
    std::cout << "Installing Python++ file associations...\n";
    
    // Escape the executable path for use in shell command
    std::string escapedPath;
    for (char c : exePath) {
        if (c == '"') {
            escapedPath += "\\\"";
        } else if (c == '\\') {
            escapedPath += "\\\\";
        } else {
            escapedPath += c;
        }
    }
    
    // Create file association for .py+ extension
    std::string assocCmd = "assoc .py+=PythonPlusPlus";
    std::cout << "Running: " << assocCmd << "\n";
    int result = system(assocCmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to associate .py+ extension\n";
        return 1;
    }
    
    // Set the file type command
    std::string ftypeCmd = "ftype PythonPlusPlus=\"" + escapedPath + "\" \"%1\" %*";
    std::cout << "Running: " << ftypeCmd << "\n";
    result = system(ftypeCmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to set file type handler\n";
        return 1;
    }
    
    std::cout << "\nFile associations installed successfully!\n";
    std::cout << "You can now double-click .py+ files to run them.\n";
    
    return 0;
}

// Uninstall file associations on Windows
int uninstallFileAssociations() {
    if (!isRunningAsAdmin()) {
        std::cerr << "Error: Administrator privileges required for uninstallation.\n";
        std::cerr << "Please run this command from an Administrator command prompt.\n";
        return 1;
    }
    
    std::cout << "Removing Python++ file associations...\n";
    
    // Remove file type handler
    std::string ftypeCmd = "ftype PythonPlusPlus=";
    std::cout << "Running: " << ftypeCmd << "\n";
    system(ftypeCmd.c_str());
    
    // Remove file association
    std::string assocCmd = "assoc .py+=";
    std::cout << "Running: " << assocCmd << "\n";
    system(assocCmd.c_str());
    
    std::cout << "\nFile associations removed successfully!\n";
    
    return 0;
}
#endif

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
    
#ifdef _WIN32
    if (firstArg == "--install") {
        return installFileAssociations();
    }
    
    if (firstArg == "--uninstall") {
        return uninstallFileAssociations();
    }
#else
    if (firstArg == "--install" || firstArg == "--uninstall") {
        std::cerr << "Error: File association management is only supported on Windows\n";
        return 1;
    }
#endif

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
