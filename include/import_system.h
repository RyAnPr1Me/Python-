#pragma once
#include "runtime.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <dlfcn.h>  // For dynamic library loading

namespace pyplusplus {

// Module metadata for compiled modules
struct ModuleMetadata {
    std::string name;
    std::string version;
    std::vector<std::string> dependencies;
    std::vector<std::string> exported_functions;
    std::vector<std::string> exported_classes;
    std::vector<std::string> exported_variables;
    std::string entry_point;
    bool is_package;
    std::filesystem::file_time_type modification_time;
};

// Import cache entry
struct ImportCacheEntry {
    std::unique_ptr<PyModule> module;
    ModuleMetadata metadata;
    std::filesystem::path file_path;
    bool is_loaded;
    uint64_t load_count;
    
    ImportCacheEntry() : is_loaded(false), load_count(0) {}
};

// Module finder interface
class ModuleFinder {
public:
    virtual ~ModuleFinder() = default;
    virtual std::filesystem::path findModule(const std::string& name) = 0;
    virtual std::vector<std::string> listModules() = 0;
    virtual bool isValidModule(const std::filesystem::path& path) = 0;
};

// File system module finder
class FileSystemModuleFinder : public ModuleFinder {
private:
    std::vector<std::filesystem::path> search_paths;
    std::vector<std::string> extensions;
    
public:
    FileSystemModuleFinder();
    
    void addSearchPath(const std::filesystem::path& path);
    void removeSearchPath(const std::filesystem::path& path);
    const std::vector<std::filesystem::path>& getSearchPaths() const { return search_paths; }
    
    std::filesystem::path findModule(const std::string& name) override;
    std::vector<std::string> listModules() override;
    bool isValidModule(const std::filesystem::path& path) override;
    
private:
    std::filesystem::path searchInPath(const std::filesystem::path& search_path, const std::string& name);
    bool hasValidExtension(const std::filesystem::path& path);
};

// Compiled module loader
class CompiledModuleLoader {
private:
    std::unordered_map<std::string, std::unique_ptr<ImportCacheEntry>> import_cache;
    std::unique_ptr<ModuleFinder> module_finder;
    std::mutex cache_mutex;
    
    // Loading strategies
    std::unique_ptr<PyModule> loadNativeModule(const std::filesystem::path& path, const std::string& name);
    std::unique_ptr<PyModule> loadBytecodeModule(const std::filesystem::path& path, const std::string& name);
    std::unique_ptr<PyModule> loadAOTModule(const std::filesystem::path& path, const std::string& name);
    
    // Metadata handling
    ModuleMetadata readMetadata(const std::filesystem::path& path);
    bool writeMetadata(const std::filesystem::path& path, const ModuleMetadata& metadata);
    bool validateMetadata(const ModuleMetadata& metadata, const std::filesystem::path& path);
    
    // Cache management
    ImportCacheEntry* getCacheEntry(const std::string& name);
    void setCacheEntry(const std::string& name, std::unique_ptr<ImportCacheEntry> entry);
    void removeCacheEntry(const std::string& name);
    void clearCache();
    
public:
    CompiledModuleLoader();
    ~CompiledModuleLoader();
    
    // Module loading
    PyModule* importModule(const std::string& name);
    PyModule* importModule(const std::string& name, const std::filesystem::path& path);
    bool reloadModule(const std::string& name);
    void unloadModule(const std::string& name);
    
    // Module discovery
    std::vector<std::string> listAvailableModules();
    bool isModuleAvailable(const std::string& name);
    std::filesystem::path getModulePath(const std::string& name);
    
    // Cache management
    void clearImportCache();
    size_t getCacheSize() const;
    std::vector<std::string> getCachedModules() const;
    
    // Configuration
    void setModuleFinder(std::unique_ptr<ModuleFinder> finder);
    ModuleFinder* getModuleFinder() const { return module_finder.get(); }
    
    // Statistics
    struct ImportStats {
        uint64_t total_imports;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t reloads;
        double average_load_time;
    };
    
    ImportStats getImportStatistics() const;
    void resetStatistics();
};

// Import hook system
class ImportHook {
public:
    virtual ~ImportHook() = default;
    virtual bool canHandle(const std::string& name) = 0;
    virtual PyModule* loadModule(const std::string& name) = 0;
    virtual int getPriority() const { return 0; }  // Higher priority = called first
};

class ImportHookManager {
private:
    std::vector<std::unique_ptr<ImportHook>> hooks;
    std::mutex hooks_mutex;
    
public:
    void addHook(std::unique_ptr<ImportHook> hook);
    void removeHook(ImportHook* hook);
    PyModule* tryHooks(const std::string& name);
    std::vector<ImportHook*> getHooks() const;
    void clearHooks();
};

// Package management
class PackageManager {
private:
    std::unordered_map<std::string, std::vector<std::string>> package_modules;
    std::filesystem::path package_cache_dir;
    
public:
    PackageManager();
    
    // Package operations
    bool installPackage(const std::string& name, const std::filesystem::path& source);
    bool uninstallPackage(const std::string& name);
    bool updatePackage(const std::string& name);
    
    // Package information
    std::vector<std::string> getPackageModules(const std::string& package_name);
    bool isPackageInstalled(const std::string& name);
    std::vector<std::string> listInstalledPackages();
    
    // Package discovery
    void scanForPackages();
    void addPackagePath(const std::filesystem::path& path);
    
    // Cache management
    void clearPackageCache();
    std::filesystem::path getPackageCacheDir() const { return package_cache_dir; }
};

// Main import system
class ImportSystem {
private:
    static std::unique_ptr<ImportSystem> instance;
    static std::mutex instance_mutex;
    
    std::unique_ptr<CompiledModuleLoader> module_loader;
    std::unique_ptr<ImportHookManager> hook_manager;
    std::unique_ptr<PackageManager> package_manager;
    std::unordered_map<std::string, PyModule*> loaded_modules;
    std::mutex modules_mutex;
    
    // Import configuration
    std::vector<std::filesystem::path> sys_path;
    bool verbose_imports;
    bool enable_bytecode_cache;
    bool enable_aot_compilation;
    
    ImportSystem();
    
public:
    ~ImportSystem();
    
    // Singleton access
    static ImportSystem& getInstance();
    static void initialize();
    static void shutdown();
    
    // Main import interface
    PyModule* import(const std::string& name);
    PyModule* import(const std::string& name, const std::filesystem::path& path);
    bool reload(const std::string& name);
    void unload(const std::string& name);
    
    // Module management
    std::vector<std::string> listLoadedModules();
    bool isLoaded(const std::string& name);
    PyModule* getLoadedModule(const std::string& name);
    
    // Path management
    void addPath(const std::filesystem::path& path);
    void removePath(const std::filesystem::path& path);
    const std::vector<std::filesystem::path>& getSysPath() const { return sys_path; }
    void clearPaths();
    
    // Hook management
    void addImportHook(std::unique_ptr<ImportHook> hook);
    void removeImportHook(ImportHook* hook);
    
    // Configuration
    void setVerboseImports(bool verbose) { verbose_imports = verbose; }
    void setBytecodeCacheEnabled(bool enabled) { enable_bytecode_cache = enabled; }
    void setAOTCompilationEnabled(bool enabled) { enable_aot_compilation = enabled; }
    
    // Statistics and debugging
    struct ImportSystemStats {
        size_t loaded_modules_count;
        size_t cache_size;
        uint64_t total_imports;
        double total_import_time;
        std::vector<std::string> most_imported_modules;
    };
    
    ImportSystemStats getStatistics() const;
    void dumpStatistics() const;
    void resetStatistics();
    
    // Advanced features
    bool preloadModule(const std::string& name);
    void preloadModules(const std::vector<std::string>& names);
    bool compileAndCache(const std::string& name);
    void invalidateCache(const std::string& name);
    void invalidateAllCaches();
};

// Built-in import hooks
class BuiltinModuleHook : public ImportHook {
public:
    bool canHandle(const std::string& name) override;
    PyModule* loadModule(const std::string& name) override;
    int getPriority() const override { return 100; }  // Highest priority
};

class CExtensionModuleHook : public ImportHook {
public:
    bool canHandle(const std::string& name) override;
    PyModule* loadModule(const std::string& name) override;
    int getPriority() const override { return 80; }
};

class AOTCompiledModuleHook : public ImportHook {
public:
    bool canHandle(const std::string& name) override;
    PyModule* loadModule(const std::string& name) override;
    int getPriority() const override { return 60; }
};

class BytecodeModuleHook : public ImportHook {
public:
    bool canHandle(const std::string& name) override;
    PyModule* loadModule(const std::string& name) override;
    int getPriority() const override { return 40; }
};

class SourceModuleHook : public ImportHook {
private:
    std::string compileSource(const std::filesystem::path& source_path);
    
public:
    bool canHandle(const std::string& name) override;
    PyModule* loadModule(const std::string& name) override;
    int getPriority() const override { return 20; }  // Lowest priority
};

// Utility functions
namespace import_utils {
    // Path manipulation
    std::filesystem::path makeModuleName(const std::string& name);
    std::string getModuleNameFromPath(const std::filesystem::path& path);
    bool isPackagePath(const std::filesystem::path& path);
    std::vector<std::filesystem::path> getPackageContents(const std::filesystem::path& package_path);
    
    // Module validation
    bool isValidModuleName(const std::string& name);
    bool isValidModulePath(const std::filesystem::path& path);
    bool isCompiledModule(const std::filesystem::path& path);
    bool isBytecodeModule(const std::filesystem::path& path);
    bool isSourceModule(const std::filesystem::path& path);
    
    // Cache management
    std::filesystem::path getCachePath(const std::string& name);
    bool isCacheValid(const std::filesystem::path& cache_path, const std::filesystem::path& source_path);
    void clearCacheForModule(const std::string& name);
    void clearAllCaches();
    
    // Dependency resolution
    std::vector<std::string> getModuleDependencies(const std::string& name);
    std::vector<std::string> resolveDependencies(const std::vector<std::string>& modules);
    bool hasCircularDependency(const std::string& name, std::vector<std::string>& visited);
}

} // namespace pyplusplus