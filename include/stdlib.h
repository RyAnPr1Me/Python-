#pragma once
#include "runtime.h"
#include <unordered_map>
#include <functional>
#include <vector>

namespace pyplusplus {

// Built-in function registry
class BuiltinRegistry {
private:
    std::unordered_map<std::string, std::function<PyObject*(const std::vector<PyObject*>&)>> builtins;
    
public:
    static BuiltinRegistry& getInstance();
    
    void registerBuiltin(const std::string& name, 
                          std::function<PyObject*(const std::vector<PyObject*>&)> func);
    
    PyObject* callBuiltin(const std::string& name, const std::vector<PyObject*>& args);
    
    bool hasBuiltin(const std::string& name) const;
    std::vector<std::string> getBuiltinNames() const;
    
private:
    BuiltinRegistry();
    void initializeBuiltins();
};

// Standard library modules
class StdLib {
public:
    // Math module
    namespace math {
        PyObject* abs_func(const std::vector<PyObject*>& args);
        PyObject* round_func(const std::vector<PyObject*>& args);
        PyObject* min_func(const std::vector<PyObject*>& args);
        PyObject* max_func(const std::vector<PyObject*>& args);
        PyObject* sum_func(const std::vector<PyObject*>& args);
        PyObject* pow_func(const std::vector<PyObject*>& args);
    }
    
    // String operations
    namespace string {
        PyObject* str_func(const std::vector<PyObject*>& args);
        PyObject* len_func(const std::vector<PyObject*>& args);
        PyObject* upper_func(const std::vector<PyObject*>& args);
        PyObject* lower_func(const std::vector<PyObject*>& args);
        PyObject* strip_func(const std::vector<PyObject*>& args);
        PyObject* split_func(const std::vector<PyObject*>& args);
        PyObject* join_func(const std::vector<PyObject*>& args);
    }
    
    // List operations
    namespace list_ops {
        PyObject* list_func(const std::vector<PyObject*>& args);
        PyObject* append_func(const std::vector<PyObject*>& args);
        PyObject* extend_func(const std::vector<PyObject*>& args);
        PyObject* pop_func(const std::vector<PyObject*>& args);
        PyObject* remove_func(const std::vector<PyObject*>& args);
        PyObject* sort_func(const std::vector<PyObject*>& args);
    }
    
    // Dictionary operations
    namespace dict_ops {
        PyObject* dict_func(const std::vector<PyObject*>& args);
        PyObject* keys_func(const std::vector<PyObject*>& args);
        PyObject* values_func(const std::vector<PyObject*>& args);
        PyObject* items_func(const std::vector<PyObject*>& args);
        PyObject* get_func(const std::vector<PyObject*>& args);
        PyObject* update_func(const std::vector<PyObject*>& args);
    }
    
    // I/O operations
    namespace io {
        PyObject* print_func(const std::vector<PyObject*>& args);
        PyObject* input_func(const std::vector<PyObject*>& args);
        PyObject* open_func(const std::vector<PyObject*>& args);
        PyObject* read_func(const std::vector<PyObject*>& args);
        PyObject* write_func(const std::vector<PyObject*>& args);
    }
    
    // System operations
    namespace system {
        PyObject* exit_func(const std::vector<PyObject*>& args);
        PyObject* getenv_func(const std::vector<PyObject*>& args);
        PyObject* time_func(const std::vector<PyObject*>& args);
        PyObject* sleep_func(const std::vector<PyObject*>& args);
    }
    
    // Type conversion
    namespace types {
        PyObject* int_func(const std::vector<PyObject*>& args);
        PyObject* float_func(const std::vector<PyObject*>& args);
        PyObject* bool_func(const std::vector<PyObject*>& args);
        PyObject* str_func(const std::vector<PyObject*>& args);
        PyObject* type_func(const std::vector<PyObject*>& args);
        PyObject* isinstance_func(const std::vector<PyObject*>& args);
    }
    
    // Iteration and ranges
    namespace iteration {
        PyObject* range_func(const std::vector<PyObject*>& args);
        PyObject* enumerate_func(const std::vector<PyObject*>& args);
        PyObject* zip_func(const std::vector<PyObject*>& args);
        PyObject* map_func(const std::vector<PyObject*>& args);
        PyObject* filter_func(const std::vector<PyObject*>& args);
    }
    
    // Exception handling
    namespace exceptions {
        PyObject* raise_func(const std::vector<PyObject*>& args);
        PyObject* assert_func(const std::vector<PyObject*>& args);
        PyObject* try_func(const std::vector<PyObject*>& args);
    }
    
    // Module initialization
    static void initialize();
};

// File I/O wrapper
class PyFile : public PyObject {
private:
    std::FILE* file_handle;
    std::string filename;
    std::string mode;
    
public:
    PyFile(const std::string& filename, const std::string& mode);
    ~PyFile();
    
    std::FILE* getFileHandle() const { return file_handle; }
    const std::string& getFilename() const { return filename; }
    const std::string& getMode() const { return mode; }
    
    PyObject* read(const std::vector<PyObject*>& args);
    PyObject* write(const std::vector<PyObject*>& args);
    PyObject* close(const std::vector<PyObject*>& args);
    PyObject* readline(const std::vector<PyObject*>& args);
    PyObject* readlines(const std::vector<PyObject*>& args);
    
    static PyType& getStaticType();
    static PyFile* create(const std::string& filename, const std::string& mode);
};

// Module system
class ModuleRegistry {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, PyObject*>> modules;
    
public:
    static ModuleRegistry& getInstance();
    
    void registerModule(const std::string& name, 
                        const std::unordered_map<std::string, PyObject*>& contents);
    
    PyObject* getModuleAttribute(const std::string& module_name, const std::string& attr_name);
    bool hasModule(const std::string& name) const;
    std::vector<std::string> getModuleNames() const;
    
private:
    ModuleRegistry();
    void initializeModules();
};

// Utility functions for argument validation
namespace utils {
    bool checkArgCount(const std::vector<PyObject*>& args, size_t expected, const std::string& func_name);
    bool checkArgCount(const std::vector<PyObject*>& args, size_t min, size_t max, const std::string& func_name);
    bool checkArgType(PyObject* arg, PyType* expected_type, const std::string& func_name, size_t arg_index);
    bool checkArgType(PyObject* arg, PyType::Kind expected_kind, const std::string& func_name, size_t arg_index);
    
    template<typename T>
    T* checkArgType(PyObject* arg, const std::string& func_name, size_t arg_index);
    
    std::string typeError(const std::string& func_name, size_t arg_index, 
                          PyType* expected_type, PyType* actual_type);
    std::string argCountError(const std::string& func_name, size_t expected, size_t actual);
    std::string argCountError(const std::string& func_name, size_t min, size_t max, size_t actual);
}

} // namespace pyplusplus