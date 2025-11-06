#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace pyplusplus {

// Forward declarations
class PyObject;
class PyType;
class PyString;
class PyList;
class PyDict;
class PyFunction;

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
};

} // namespace pyplusplus