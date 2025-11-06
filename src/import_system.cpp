#include "import_system.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

namespace pyplusplus {

// ImportSystem singleton implementation
std::unique_ptr<ImportSystem> ImportSystem::instance;
std::mutex ImportSystem::instance_mutex;

ImportSystem& ImportSystem::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance) {
        instance = std::unique_ptr<ImportSystem>(new ImportSystem());
    }
    return *instance;
}

void ImportSystem::initialize() {
    getInstance();  // Force creation
}

void ImportSystem::shutdown() {
    std::lock_guard<std::mutex> lock(instance_mutex);
    instance.reset();
}

ImportSystem::ImportSystem() 
    : module_loader(std::make_unique<CompiledModuleLoader>()),
      hook_manager(std::make_unique<ImportHookManager>()),
      package_manager(std::make_unique<PackageManager>()),
      verbose_imports(false),
      enable_bytecode_cache(true),
      enable_aot_compilation(true) {
    
    // Initialize sys.path with default paths
    sys_path.push_back(std::filesystem::current_path());
    sys_path.push_back(std::filesystem::current_path() / "lib");
    sys_path.push_back(std::filesystem::current_path() / "modules");
    
    // Register built-in import hooks
    hook_manager->addHook(std::make_unique<BuiltinModuleHook>());
    hook_manager->addHook(std::make_unique<CExtensionModuleHook>());
    hook_manager->addHook(std::make_unique<AOTCompiledModuleHook>());
    hook_manager->addHook(std::make_unique<BytecodeModuleHook>());
    hook_manager->addHook(std::make_unique<SourceModuleHook>());
    
    // Configure module finder
    auto fs_finder = std::make_unique<FileSystemModuleFinder>();
    for (const auto& path : sys_path) {
        fs_finder->addSearchPath(path);
    }
    module_loader->setModuleFinder(std::move(fs_finder));
}

ImportSystem::~ImportSystem() {
    // Unload all modules
    std::lock_guard<std::mutex> lock(modules_mutex);
    for (auto& [name, module] : loaded_modules) {
        module_loader->unloadModule(name);
    }
    loaded_modules.clear();
}

PyModule* ImportSystem::import(const std::string& name) {
    if (verbose_imports) {
        std::cout << "Importing module: " << name << std::endl;
    }
    
    // Check if already loaded
    {
        std::lock_guard<std::mutex> lock(modules_mutex);
        auto it = loaded_modules.find(name);
        if (it != loaded_modules.end()) {
            return it->second;
        }
    }
    
    // Try import hooks first
    PyModule* module = hook_manager->tryHooks(name);
    if (module) {
        std::lock_guard<std::mutex> lock(modules_mutex);
        loaded_modules[name] = module;
        return module;
    }
    
    // Fall back to module loader
    module = module_loader->importModule(name);
    if (module) {
        std::lock_guard<std::mutex> lock(modules_mutex);
        loaded_modules[name] = module;
        return module;
    }
    
    if (verbose_imports) {
        std::cerr << "Failed to import module: " << name << std::endl;
    }
    
    return nullptr;
}

PyModule* ImportSystem::import(const std::string& name, const std::filesystem::path& path) {
    if (verbose_imports) {
        std::cout << "Importing module: " << name << " from " << path << std::endl;
    }
    
    // Check if already loaded
    {
        std::lock_guard<std::mutex> lock(modules_mutex);
        auto it = loaded_modules.find(name);
        if (it != loaded_modules.end()) {
            return it->second;
        }
    }
    
    // Load from specific path
    PyModule* module = module_loader->importModule(name, path);
    if (module) {
        std::lock_guard<std::mutex> lock(modules_mutex);
        loaded_modules[name] = module;
        return module;
    }
    
    return nullptr;
}

bool ImportSystem::reload(const std::string& name) {
    if (verbose_imports) {
        std::cout << "Reloading module: " << name << std::endl;
    }
    
    bool success = module_loader->reloadModule(name);
    if (success) {
        // Update cached module reference
        PyModule* reloaded = module_loader->importModule(name);
        if (reloaded) {
            std::lock_guard<std::mutex> lock(modules_mutex);
            loaded_modules[name] = reloaded;
        }
    }
    
    return success;
}

void ImportSystem::unload(const std::string& name) {
    if (verbose_imports) {
        std::cout << "Unloading module: " << name << std::endl;
    }
    
    module_loader->unloadModule(name);
    
    std::lock_guard<std::mutex> lock(modules_mutex);
    loaded_modules.erase(name);
}

std::vector<std::string> ImportSystem::listLoadedModules() {
    std::lock_guard<std::mutex> lock(modules_mutex);
    std::vector<std::string> modules;
    modules.reserve(loaded_modules.size());
    
    for (const auto& [name, module] : loaded_modules) {
        modules.push_back(name);
    }
    
    return modules;
}

bool ImportSystem::isLoaded(const std::string& name) {
    std::lock_guard<std::mutex> lock(modules_mutex);
    return loaded_modules.find(name) != loaded_modules.end();
}

PyModule* ImportSystem::getLoadedModule(const std::string& name) {
    std::lock_guard<std::mutex> lock(modules_mutex);
    auto it = loaded_modules.find(name);
    return it != loaded_modules.end() ? it->second : nullptr;
}

void ImportSystem::addPath(const std::filesystem::path& path) {
    if (std::find(sys_path.begin(), sys_path.end(), path) == sys_path.end()) {
        sys_path.push_back(path);
        
        // Update module finder
        if (auto fs_finder = dynamic_cast<FileSystemModuleFinder*>(module_loader->getModuleFinder())) {
            fs_finder->addSearchPath(path);
        }
    }
}

void ImportSystem::removePath(const std::filesystem::path& path) {
    auto it = std::find(sys_path.begin(), sys_path.end(), path);
    if (it != sys_path.end()) {
        sys_path.erase(it);
        
        // Update module finder
        if (auto fs_finder = dynamic_cast<FileSystemModuleFinder*>(module_loader->getModuleFinder())) {
            fs_finder->removeSearchPath(path);
        }
    }
}

void ImportSystem::clearPaths() {
    sys_path.clear();
    
    // Reset module finder
    auto fs_finder = std::make_unique<FileSystemModuleFinder>();
    module_loader->setModuleFinder(std::move(fs_finder));
}

void ImportSystem::addImportHook(std::unique_ptr<ImportHook> hook) {
    hook_manager->addHook(std::move(hook));
}

void ImportSystem::removeImportHook(ImportHook* hook) {
    hook_manager->removeHook(hook);
}

ImportSystem::ImportSystemStats ImportSystem::getStatistics() const {
    ImportSystemStats stats;
    
    std::lock_guard<std::mutex> lock(modules_mutex);
    stats.loaded_modules_count = loaded_modules.size();
    stats.cache_size = module_loader->getCacheSize();
    
    // Get loader statistics
    auto loader_stats = module_loader->getImportStatistics();
    stats.total_imports = loader_stats.total_imports;
    stats.total_import_time = loader_stats.average_load_time * loader_stats.total_imports;
    
    // Find most imported modules (simplified)
    for (const auto& [name, module] : loaded_modules) {
        stats.most_imported_modules.push_back(name);
    }
    
    return stats;
}

void ImportSystem::dumpStatistics() const {
    auto stats = getStatistics();
    
    std::cout << "Import System Statistics:" << std::endl;
    std::cout << "  Loaded modules: " << stats.loaded_modules_count << std::endl;
    std::cout << "  Cache size: " << stats.cache_size << std::endl;
    std::cout << "  Total imports: " << stats.total_imports << std::endl;
    std::cout << "  Total import time: " << stats.total_import_time << "ms" << std::endl;
    
    if (!stats.most_imported_modules.empty()) {
        std::cout << "  Loaded modules:" << std::endl;
        for (const auto& name : stats.most_imported_modules) {
            std::cout << "    " << name << std::endl;
        }
    }
}

void ImportSystem::resetStatistics() {
    module_loader->resetStatistics();
}

bool ImportSystem::preloadModule(const std::string& name) {
    return import(name) != nullptr;
}

void ImportSystem::preloadModules(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        preloadModule(name);
    }
}

bool ImportSystem::compileAndCache(const std::string& name) {
    // This would trigger AOT compilation and caching
    // Implementation depends on the specific compilation strategy
    return module_loader->importModule(name) != nullptr;
}

void ImportSystem::invalidateCache(const std::string& name) {
    module_loader->removeCacheEntry(name);
}

void ImportSystem::invalidateAllCaches() {
    module_loader->clearImportCache();
}

// ImportHookManager implementation
void ImportHookManager::addHook(std::unique_ptr<ImportHook> hook) {
    std::lock_guard<std::mutex> lock(hooks_mutex);
    hooks.push_back(std::move(hook));
    
    // Sort by priority (highest first)
    std::sort(hooks.begin(), hooks.end(), 
              [](const std::unique_ptr<ImportHook>& a, const std::unique_ptr<ImportHook>& b) {
                  return a->getPriority() > b->getPriority();
              });
}

void ImportHookManager::removeHook(ImportHook* hook) {
    std::lock_guard<std::mutex> lock(hooks_mutex);
    hooks.erase(
        std::remove_if(hooks.begin(), hooks.end(),
                      [hook](const std::unique_ptr<ImportHook>& h) { return h.get() == hook; }),
        hooks.end());
}

PyModule* ImportHookManager::tryHooks(const std::string& name) {
    std::lock_guard<std::mutex> lock(hooks_mutex);
    
    for (const auto& hook : hooks) {
        if (hook->canHandle(name)) {
            PyModule* module = hook->loadModule(name);
            if (module) {
                return module;
            }
        }
    }
    
    return nullptr;
}

std::vector<ImportHook*> ImportHookManager::getHooks() const {
    std::lock_guard<std::mutex> lock(hooks_mutex);
    std::vector<ImportHook*> result;
    result.reserve(hooks.size());
    
    for (const auto& hook : hooks) {
        result.push_back(hook.get());
    }
    
    return result;
}

void ImportHookManager::clearHooks() {
    std::lock_guard<std::mutex> lock(hooks_mutex);
    hooks.clear();
}

// Built-in import hooks implementation
bool BuiltinModuleHook::canHandle(const std::string& name) {
    // List of built-in modules
    static const std::vector<std::string> builtin_modules = {
        "sys", "os", "io", "math", "time", "random", "json", "re",
        "collections", "itertools", "functools", "operator", "string",
        "datetime", "hashlib", "uuid", "base64", "urllib", "http"
    };
    
    return std::find(builtin_modules.begin(), builtin_modules.end(), name) != builtin_modules.end();
}

PyModule* BuiltinModuleHook::loadModule(const std::string& name) {
    // Create built-in module with standard library functions
    auto module = std::make_unique<PyModule>(name);
    
    // Add built-in functions and objects based on module name
    if (name == "math") {
        // Add math functions
        module->setGlobal("sin", PyFunction::create("sin", reinterpret_cast<void*>(sin), {"x"}));
        module->setGlobal("cos", PyFunction::create("cos", reinterpret_cast<void*>(cos), {"x"}));
        module->setGlobal("tan", PyFunction::create("tan", reinterpret_cast<void*>(tan), {"x"}));
        module->setGlobal("sqrt", PyFunction::create("sqrt", reinterpret_cast<void*>(sqrt), {"x"}));
        module->setGlobal("pi", PyFloat::create(3.141592653589793));
        module->setGlobal("e", PyFloat::create(2.718281828459045));
    } else if (name == "sys") {
        // Add sys module attributes
        module->setGlobal("version", PyString::create("3.10+"));
        module->setGlobal("platform", PyString::create("python++"));
    }
    // Add more built-in modules as needed...
    
    return module.release();
}

bool CExtensionModuleHook::canHandle(const std::string& name) {
    // Check for .so or .dll files
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (!path.empty()) {
        return path.extension() == ".so" || path.extension() == ".dll" || 
               path.extension() == ".dylib";
    }
    
    return false;
}

PyModule* CExtensionModuleHook::loadModule(const std::string& name) {
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (path.empty()) {
        return nullptr;
    }
    
    // Load dynamic library
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load C extension: " << dlerror() << std::endl;
        return nullptr;
    }
    
    // Look for module initialization function
    std::string init_func_name = "PyInit_" + name;
    auto init_func = reinterpret_cast<PyObject* (*)()>(dlsym(handle, init_func_name.c_str()));
    
    if (!init_func) {
        dlclose(handle);
        std::cerr << "Failed to find initialization function: " << init_func_name << std::endl;
        return nullptr;
    }
    
    // Call initialization function
    PyObject* module_obj = init_func();
    if (!module_obj) {
        dlclose(handle);
        return nullptr;
    }
    
    // Create PyModule wrapper
    auto module = std::make_unique<PyModule>(name);
    module->setModuleHandle(handle);
    
    // Copy module contents from C extension
    // This would need proper implementation based on the C extension API
    
    return module.release();
}

bool AOTCompiledModuleHook::canHandle(const std::string& name) {
    // Check for AOT compiled modules (.aot, .pyc, etc.)
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (!path.empty()) {
        return path.extension() == ".aot" || path.extension() == ".pyc" || 
               path.extension() == ".pyo";
    }
    
    return false;
}

PyModule* AOTCompiledModuleHook::loadModule(const std::string& name) {
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (path.empty()) {
        return nullptr;
    }
    
    // Load AOT compiled module
    // This would involve loading the compiled binary and setting up the module interface
    auto module = std::make_unique<PyModule>(name);
    
    // Implementation depends on the AOT compilation format
    // For now, return a basic module
    return module.release();
}

bool BytecodeModuleHook::canHandle(const std::string& name) {
    // Check for bytecode files (.pyc)
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (!path.empty()) {
        return path.extension() == ".pyc";
    }
    
    return false;
}

PyModule* BytecodeModuleHook::loadModule(const std::string& name) {
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (path.empty()) {
        return nullptr;
    }
    
    // Load and execute bytecode
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    
    // Read bytecode
    std::vector<uint8_t> bytecode((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
    
    // Execute bytecode using the runtime
    auto module = std::make_unique<PyModule>(name);
    
    // This would need proper bytecode execution implementation
    return module.release();
}

bool SourceModuleHook::canHandle(const std::string& name) {
    // Check for Python source files (.py)
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (!path.empty()) {
        return path.extension() == ".py";
    }
    
    return false;
}

PyModule* SourceModuleHook::loadModule(const std::string& name) {
    auto& import_system = ImportSystem::getInstance();
    auto path = import_system.getModulePath(name);
    
    if (path.empty()) {
        return nullptr;
    }
    
    // Compile source to bytecode, then execute
    std::string compiled_bytecode = compileSource(path);
    if (compiled_bytecode.empty()) {
        return nullptr;
    }
    
    // Execute compiled bytecode
    auto module = std::make_unique<PyModule>(name);
    
    // This would need proper compilation and execution implementation
    return module.release();
}

std::string SourceModuleHook::compileSource(const std::filesystem::path& source_path) {
    // Read source file
    std::ifstream file(source_path);
    if (!file.is_open()) {
        return "";
    }
    
    std::string source((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    
    // Compile to bytecode (simplified)
    // This would need proper Python compilation implementation
    return source;
}

// Utility functions implementation
namespace import_utils {
    std::filesystem::path makeModuleName(const std::string& name) {
        std::string module_name = name;
        std::replace(module_name.begin(), module_name.end(), '.', '/');
        return std::filesystem::path(module_name);
    }
    
    std::string getModuleNameFromPath(const std::filesystem::path& path) {
        std::string name = path.stem().string();
        std::replace(name.begin(), name.end(), '/', '.');
        return name;
    }
    
    bool isValidModuleName(const std::string& name) {
        if (name.empty()) return false;
        
        // Check for valid characters
        for (char c : name) {
            if (!std::isalnum(c) && c != '_' && c != '.') {
                return false;
            }
        }
        
        // Check if it doesn't start with a number
        if (std::isdigit(name[0])) {
            return false;
        }
        
        return true;
    }
    
    bool isCompiledModule(const std::filesystem::path& path) {
        return path.extension() == ".so" || path.extension() == ".dll" || 
               path.extension() == ".dylib" || path.extension() == ".aot";
    }
    
    bool isBytecodeModule(const std::filesystem::path& path) {
        return path.extension() == ".pyc" || path.extension() == ".pyo";
    }
    
    bool isSourceModule(const std::filesystem::path& path) {
        return path.extension() == ".py";
    }
}

} // namespace pyplusplus