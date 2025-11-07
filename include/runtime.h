#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace pyplusplus {

// Forward declarations
class PyObject;
class PyType;
class PyString;
class PyList;
class PyDict;
class PyFunction;
class PyModule;
class PyFrame;
class PyThreadState;
class PyInterpreter;

// Object header - common to all Python objects
struct ObjectHeader {
    PyType* type;
    uint32_t ref_count;
    uint32_t flags;
    
    ObjectHeader(PyType* type) : type(type), ref_count(1), flags(0) {}
};

// Type object
class PyType {
public:
    enum class Kind {
        NONE,
        BOOL,
        INT,
        FLOAT,
        STRING,
        LIST,
        DICT,
        FUNCTION,
        CLASS,
        MODULE
    };
    
private:
    ObjectHeader header;
    std::string name;
    Kind kind;
    size_t object_size;
    
    // Type methods
    PyObject* (*init_func)(PyObject* obj, const std::vector<PyObject*>& args);
    void (*dealloc_func)(PyObject* obj);
    std::string (*repr_func)(const PyObject* obj);
    PyObject* (*call_func)(PyObject* obj, const std::vector<PyObject*>& args);
    PyObject* (*getattr_func)(PyObject* obj, const std::string& name);
    void (*setattr_func)(PyObject* obj, const std::string& name, PyObject* value);
    PyObject* (*getitem_func)(PyObject* obj, PyObject* key);
    void (*setitem_func)(PyObject* obj, PyObject* key, PyObject* value);
    
public:
    PyType(const std::string& name, Kind kind, size_t object_size);
    
    const std::string& getName() const { return name; }
    Kind getKind() const { return kind; }
    size_t getObjectSize() const { return object_size; }
    
    // Method setters
    void setInitFunc(PyObject* (*func)(PyObject*, const std::vector<PyObject*>&)) { init_func = func; }
    void setDeallocFunc(void (*func)(PyObject*)) { dealloc_func = func; }
    void setReprFunc(std::string (*func)(const PyObject*)) { repr_func = func; }
    void setCallFunc(PyObject* (*func)(PyObject*, const std::vector<PyObject*>&)) { call_func = func; }
    void setGetattrFunc(PyObject* (*func)(PyObject*, const std::string&)) { getattr_func = func; }
    void setSetattrFunc(void (*func)(PyObject*, const std::string&, PyObject*)) { setattr_func = func; }
    void setGetitemFunc(PyObject* (*func)(PyObject*, PyObject*)) { getitem_func = func; }
    void setSetitemFunc(void (*func)(PyObject*, PyObject*, PyObject*)) { setitem_func = func; }
    
    // Method invokers
    PyObject* init(PyObject* obj, const std::vector<PyObject*>& args) const {
        return init_func ? init_func(obj, args) : obj;
    }
    
    void dealloc(PyObject* obj) const {
        if (dealloc_func) dealloc_func(obj);
    }
    
    std::string repr(const PyObject* obj) const {
        return repr_func ? repr_func(obj) : "<object>";
    }
    
    PyObject* call(PyObject* obj, const std::vector<PyObject*>& args) const {
        return call_func ? call_func(obj, args) : nullptr;
    }
    
    PyObject* getattr(PyObject* obj, const std::string& name) const {
        return getattr_func ? getattr_func(obj, name) : nullptr;
    }
    
    void setattr(PyObject* obj, const std::string& name, PyObject* value) const {
        if (setattr_func) setattr_func(obj, name, value);
    }
    
    PyObject* getitem(PyObject* obj, PyObject* key) const {
        return getitem_func ? getitem_func(obj, key) : nullptr;
    }
    
    void setitem(PyObject* obj, PyObject* key, PyObject* value) const {
        if (setitem_func) setitem_func(obj, key, value);
    }
};

// Base Python object
class PyObject {
private:
    ObjectHeader header;
    
protected:
    explicit PyObject(PyType* type) : header(type) {}
    
public:
    virtual ~PyObject() = default;
    
    // Reference counting
    void ref() { header.ref_count++; }
    void unref() { 
        if (--header.ref_count == 0) {
            header.type->dealloc(this);
            delete this;
        }
    }
    
    uint32_t getRefCount() const { return header.ref_count; }
    PyType* getType() const { return header.type; }
    
    // Type methods
    PyObject* init(const std::vector<PyObject*>& args) {
        return header.type->init(this, args);
    }
    
    std::string repr() const {
        return header.type->repr(this);
    }
    
    PyObject* call(const std::vector<PyObject*>& args) {
        return header.type->call(this, args);
    }
    
    PyObject* getattr(const std::string& name) {
        return header.type->getattr(this, name);
    }
    
    void setattr(const std::string& name, PyObject* value) {
        header.type->setattr(this, name, value);
    }
    
    PyObject* getitem(PyObject* key) {
        return header.type->getitem(this, key);
    }
    
    void setitem(PyObject* key, PyObject* value) {
        header.type->setitem(this, key, value);
    }
    
    // Type checking
    bool isInstance(PyType* type) const { return header.type == type; }
    bool isNone() const { return header.type->getKind() == PyType::Kind::NONE; }
    bool isBool() const { return header.type->getKind() == PyType::Kind::BOOL; }
    bool isInt() const { return header.type->getKind() == PyType::Kind::INT; }
    bool isFloat() const { return header.type->getKind() == PyType::Kind::FLOAT; }
    bool isString() const { return header.type->getKind() == PyType::Kind::STRING; }
    bool isList() const { return header.type->getKind() == PyType::Kind::LIST; }
    bool isDict() const { return header.type->getKind() == PyType::Kind::DICT; }
    bool isFunction() const { return header.type->getKind() == PyType::Kind::FUNCTION; }
    
    // Downcasting helpers
    template<typename T>
    T* as() {
        return dynamic_cast<T*>(this);
    }
    
    template<typename T>
    const T* as() const {
        return dynamic_cast<const T*>(this);
    }
};

// None type
class PyNone : public PyObject {
public:
    PyNone() : PyObject(&getStaticType()) {}
    
    static PyType& getStaticType();
    static PyNone* getInstance();
};

// Boolean type
class PyBool : public PyObject {
private:
    bool value;
    
public:
    PyBool(bool value) : PyObject(&getStaticType()), value(value) {}
    
    bool getValue() const { return value; }
    
    static PyType& getStaticType();
    static PyBool* getInstance(bool value);
};

// Integer type
class PyInt : public PyObject {
private:
    int64_t value;
    
public:
    PyInt(int64_t value) : PyObject(&getStaticType()), value(value) {}
    
    int64_t getValue() const { return value; }
    void setValue(int64_t new_value) { value = new_value; }
    
    static PyType& getStaticType();
    static PyInt* create(int64_t value);
};

// Float type
class PyFloat : public PyObject {
private:
    double value;
    
public:
    PyFloat(double value) : PyObject(&getStaticType()), value(value) {}
    
    double getValue() const { return value; }
    void setValue(double new_value) { value = new_value; }
    
    static PyType& getStaticType();
    static PyFloat* create(double value);
};

// String type
class PyString : public PyObject {
private:
    std::string value;
    
public:
    PyString(const std::string& value) : PyObject(&getStaticType()), value(value) {}
    
    const std::string& getValue() const { return value; }
    void setValue(const std::string& new_value) { value = new_value; }
    
    size_t length() const { return value.length(); }
    
    static PyType& getStaticType();
    static PyString* create(const std::string& value);
};

// List type
class PyList : public PyObject {
private:
    std::vector<PyObject*> items;
    
public:
    PyList() : PyObject(&getStaticType()) {}
    PyList(const std::vector<PyObject*>& items) : PyObject(&getStaticType()), items(items) {
        for (auto item : items) item->ref();
    }
    
    ~PyList() {
        for (auto item : items) item->unref();
    }
    
    size_t size() const { return items.size(); }
    PyObject* get(size_t index) const { 
        return index < items.size() ? items[index] : nullptr; 
    }
    
    void set(size_t index, PyObject* value) {
        if (index < items.size()) {
            items[index]->unref();
            items[index] = value;
            value->ref();
        }
    }
    
    void append(PyObject* value) {
        items.push_back(value);
        value->ref();
    }
    
    const std::vector<PyObject*>& getItems() const { return items; }
    
    static PyType& getStaticType();
    static PyList* create();
    static PyList* create(const std::vector<PyObject*>& items);
};

// Dictionary type
class PyDict : public PyObject {
private:
    std::unordered_map<std::string, PyObject*> items;
    
public:
    PyDict() : PyObject(&getStaticType()) {}
    
    ~PyDict() {
        for (auto& [key, value] : items) {
            value->unref();
        }
    }
    
    size_t size() const { return items.size(); }
    PyObject* get(const std::string& key) const {
        auto it = items.find(key);
        return it != items.end() ? it->second : nullptr;
    }
    
    void set(const std::string& key, PyObject* value) {
        auto it = items.find(key);
        if (it != items.end()) {
            it->second->unref();
        }
        items[key] = value;
        value->ref();
    }
    
    bool contains(const std::string& key) const {
        return items.find(key) != items.end();
    }
    
    const std::unordered_map<std::string, PyObject*>& getItems() const { return items; }
    
    static PyType& getStaticType();
    static PyDict* create();
};

// Function type
class PyFunction : public PyObject {
private:
    std::string name;
    void* function_ptr;  // Pointer to compiled function
    std::vector<std::string> parameter_names;
    bool is_variadic;
    
public:
    PyFunction(const std::string& name, void* function_ptr, 
                const std::vector<std::string>& parameter_names, bool is_variadic = false)
        : PyObject(&getStaticType()), name(name), function_ptr(function_ptr), 
          parameter_names(parameter_names), is_variadic(is_variadic) {}
    
    const std::string& getName() const { return name; }
    void* getFunctionPtr() const { return function_ptr; }
    const std::vector<std::string>& getParameterNames() const { return parameter_names; }
    bool isVariadic() const { return is_variadic; }
    
    static PyType& getStaticType();
    static PyFunction* create(const std::string& name, void* function_ptr, 
                               const std::vector<std::string>& parameter_names, bool is_variadic = false);
};

// Memory management
class MemoryManager {
private:
    static std::vector<std::unique_ptr<PyObject[]>> allocations;
    static std::vector<PyObject*> objects;
    
public:
    static void* allocate(size_t size);
    static void deallocate(void* ptr);
    static void collectGarbage();
    static size_t getTotalAllocated();
    static size_t getObjectCount();
};

// Exception handling
class PyException : public std::exception {
private:
    std::string message;
    PyObject* exception_type;
    
public:
    PyException(const std::string& message, PyObject* exception_type = nullptr)
        : message(message), exception_type(exception_type) {}
    
    const char* what() const noexcept override { return message.c_str(); }
    PyObject* getExceptionType() const { return exception_type; }
};

// Frame object for execution context
class PyFrame {
private:
    PyFrame* previous;
    PyFunction* function;
    PyObject* locals;
    PyObject* globals;
    PyObject* builtins;
    std::vector<PyObject*> stack;
    size_t stack_pointer;
    uint32_t instruction_pointer;
    bool is_executing;
    
public:
    PyFrame(PyFunction* func, PyObject* globals, PyObject* builtins, PyFrame* prev = nullptr);
    ~PyFrame();
    
    // Stack operations
    void push(PyObject* obj);
    PyObject* pop();
    PyObject* peek(size_t offset = 0);
    void clearStack();
    
    // Execution control
    void setInstructionPointer(uint32_t ip) { instruction_pointer = ip; }
    uint32_t getInstructionPointer() const { return instruction_pointer; }
    void advanceInstructionPointer(uint32_t offset = 1) { instruction_pointer += offset; }
    
    // Accessors
    PyFrame* getPrevious() const { return previous; }
    PyFunction* getFunction() const { return function; }
    PyObject* getLocals() const { return locals; }
    PyObject* getGlobals() const { return globals; }
    PyObject* getBuiltins() const { return builtins; }
    bool isExecuting() const { return is_executing; }
    void setExecuting(bool executing) { is_executing = executing; }
    
    // Variable access
    PyObject* getLocal(const std::string& name);
    void setLocal(const std::string& name, PyObject* value);
    PyObject* getGlobal(const std::string& name);
    void setGlobal(const std::string& name, PyObject* value);
};

// Thread state for multi-threading
class PyThreadState {
private:
    std::thread::id thread_id;
    PyFrame* current_frame;
    PyObject* current_exception;
    std::recursive_mutex gil_mutex;
    std::atomic<bool> gil_held;
    
public:
    PyThreadState();
    ~PyThreadState();
    
    // Frame management
    void pushFrame(PyFrame* frame);
    void popFrame();
    PyFrame* getCurrentFrame() const { return current_frame; }
    
    // Exception handling
    void setException(PyObject* exception);
    PyObject* getException() const { return current_exception; }
    void clearException();
    bool hasException() const { return current_exception != nullptr; }
    
    // GIL (Global Interpreter Lock) management
    void acquireGIL();
    void releaseGIL();
    bool hasGIL() const { return gil_held; }
    
    // Thread identification
    std::thread::id getThreadId() const { return thread_id; }
    
    // Static thread management
    static PyThreadState* getCurrent();
    static void setCurrent(PyThreadState* state);
    static std::vector<PyThreadState*> getAllThreads();
};

// Module system for AOT compilation
class PyModule {
private:
    std::string name;
    PyObject* globals;
    void* module_handle;  // For dynamic libraries
    bool is_loaded;
    
public:
    PyModule(const std::string& name);
    ~PyModule();
    
    // Module lifecycle
    bool load(const std::string& path);
    bool unload();
    bool isLoaded() const { return is_loaded; }
    
    // Symbol resolution
    void* getSymbol(const std::string& name);
    PyFunction* getFunction(const std::string& name);
    PyObject* getGlobal(const std::string& name);
    
    // Global variable access
    void setGlobal(const std::string& name, PyObject* value);
    PyObject* getGlobals() const { return globals; }
    
    // Module information
    const std::string& getName() const { return name; }
    void* getModuleHandle() const { return module_handle; }
    
    // Static module management
    static PyModule* import(const std::string& name);
    static PyModule* getImported(const std::string& name);
    static std::vector<PyModule*> getAllImported();
};

// Enhanced runtime for AOT compilation
class AOTRuntime {
private:
    static bool initialized;
    static std::unordered_map<std::string, PyModule*> imported_modules;
    static std::vector<PyThreadState*> thread_states;
    static std::mutex runtime_mutex;
    
public:
    // Initialization and cleanup
    static void initialize();
    static void shutdown();
    static bool isInitialized() { return initialized; }
    
    // Module management
    static PyModule* importModule(const std::string& name);
    static bool loadCompiledModule(const std::string& name, const std::string& path);
    static void unloadModule(const std::string& name);
    
    // Thread management
    static PyThreadState* createThreadState();
    static void destroyThreadState(PyThreadState* state);
    static PyThreadState* getCurrentThread();
    
    // Execution
    static PyObject* executeFunction(PyFunction* function, const std::vector<PyObject*>& args);
    static PyObject* executeModule(const std::string& module_name, const std::string& function_name);
    
    // Memory management
    static void collectGarbage();
    static size_t getMemoryUsage();
    static void setMemoryLimit(size_t limit_bytes);
    
    // Profiling and debugging
    static void enableProfiling(bool enable);
    static void dumpProfilingData();
    static void enableDebugMode(bool enable);
    
    // JIT compilation (for hot paths)
    static void* compileFunction(const std::string& bytecode, size_t length);
    static bool isJITEnabled();
    static void enableJIT(bool enable);
};

// Runtime initialization
class Runtime {
private:
    static bool initialized;
    
public:
    static void initialize();
    static void shutdown();
    static bool isInitialized() { return initialized; }
    
    // Built-in types
    static PyType* NoneType;
    static PyType* BoolType;
    static PyType* IntType;
    static PyType* FloatType;
    static PyType* StringType;
    static PyType* ListType;
    static PyType* DictType;
    static PyType* FunctionType;
    
    // Built-in objects
    static PyNone* NoneObject;
    static PyBool* TrueObject;
    static PyBool* FalseObject;
    
    // AOT runtime integration
    static AOTRuntime* getAOTRuntime();
};

// C API compatibility layer
extern "C" {
    // Object management
    void* Py_IncRef(void* obj);
    void Py_DecRef(void* obj);
    void* PyObject_Malloc(size_t size);
    void PyObject_Free(void* ptr);
    
    // Type creation
    void* PyLong_FromLong(long value);
    void* PyFloat_FromDouble(double value);
    void* PyUnicode_FromString(const char* str);
    
    // Operations
    void* PyNumber_Add(void* left, void* right);
    void* PyNumber_Subtract(void* left, void* right);
    void* PyNumber_Multiply(void* left, void* right);
    void* PyNumber_TrueDivide(void* left, void* right);
    void* PyObject_RichCompare(void* left, void* right, int opid);
    
    // Function calls
    void* PyObject_Call(void* callable, void* args, void* kwargs);
    void* PyObject_CallFunction(void* callable, const char* format, ...);
    
    // Exception handling
    void* PyErr_Occurred();
    void PyErr_SetString(void* exception_type, const char* message);
    void PyErr_Clear();
    
    // Module system
    void* PyImport_ImportModule(const char* name);
    void* PyModule_GetDict(void* module);
    
    // Runtime initialization
    void Py_Initialize();
    void Py_Finalize();
    int Py_IsInitialized();
}

} // namespace pyplusplus