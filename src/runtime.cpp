#include "runtime.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace pyplusplus {

// Static member definitions
std::vector<std::unique_ptr<PyObject[]>> MemoryManager::allocations;
std::vector<PyObject*> MemoryManager::objects;

bool Runtime::initialized = false;
PyType* Runtime::NoneType = nullptr;
PyType* Runtime::BoolType = nullptr;
PyType* Runtime::IntType = nullptr;
PyType* Runtime::FloatType = nullptr;
PyType* Runtime::StringType = nullptr;
PyType* Runtime::ListType = nullptr;
PyType* Runtime::DictType = nullptr;
PyType* Runtime::FunctionType = nullptr;
PyNone* Runtime::NoneObject = nullptr;
PyBool* Runtime::TrueObject = nullptr;
PyBool* Runtime::FalseObject = nullptr;

// PyType implementation
PyType::PyType(const std::string& name, Kind kind, size_t object_size)
    : header(nullptr), name(name), kind(kind), object_size(object_size),
      init_func(nullptr), dealloc_func(nullptr), repr_func(nullptr),
      call_func(nullptr), getattr_func(nullptr), setattr_func(nullptr),
      getitem_func(nullptr), setitem_func(nullptr) {}

// PyType static instances and methods
PyType& PyNone::getStaticType() {
    static PyType type("NoneType", PyType::Kind::NONE, sizeof(PyNone));
    return type;
}

PyNone* PyNone::getInstance() {
    static PyNone instance;
    return &instance;
}

PyType& PyBool::getStaticType() {
    static PyType type("bool", PyType::Kind::BOOL, sizeof(PyBool));
    return type;
}

PyBool* PyBool::getInstance(bool value) {
    static PyBool true_instance(true);
    static PyBool false_instance(false);
    return value ? &true_instance : &false_instance;
}

PyType& PyInt::getStaticType() {
    static PyType type("int", PyType::Kind::INT, sizeof(PyInt));
    return type;
}

PyInt* PyInt::create(int64_t value) {
    return new PyInt(value);
}

PyType& PyFloat::getStaticType() {
    static PyType type("float", PyType::Kind::FLOAT, sizeof(PyFloat));
    return type;
}

PyFloat* PyFloat::create(double value) {
    return new PyFloat(value);
}

PyType& PyString::getStaticType() {
    static PyType type("str", PyType::Kind::STRING, sizeof(PyString));
    return type;
}

PyString* PyString::create(const std::string& value) {
    return new PyString(value);
}

PyType& PyList::getStaticType() {
    static PyType type("list", PyType::Kind::LIST, sizeof(PyList));
    return type;
}

PyList* PyList::create() {
    return new PyList();
}

PyList* PyList::create(const std::vector<PyObject*>& items) {
    return new PyList(items);
}

PyType& PyDict::getStaticType() {
    static PyType type("dict", PyType::Kind::DICT, sizeof(PyDict));
    return type;
}

PyDict* PyDict::create() {
    return new PyDict();
}

PyType& PyFunction::getStaticType() {
    static PyType type("function", PyType::Kind::FUNCTION, sizeof(PyFunction));
    return type;
}

PyFunction* PyFunction::create(const std::string& name, void* function_ptr, 
                                const std::vector<std::string>& parameter_names, bool is_variadic) {
    return new PyFunction(name, function_ptr, parameter_names, is_variadic);
}

// Memory management implementation
void* MemoryManager::allocate(size_t size) {
    // For simplicity, use new/delete. In a real implementation, 
    // we'd use a custom allocator with garbage collection.
    void* ptr = ::operator new(size);
    return ptr;
}

void MemoryManager::deallocate(void* ptr) {
    ::operator delete(ptr);
}

void MemoryManager::collectGarbage() {
    // Simple mark-and-sweep garbage collection
    // In a real implementation, this would be much more sophisticated
    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [](PyObject* obj) { return obj->getRefCount() == 0; }),
        objects.end()
    );
}

size_t MemoryManager::getTotalAllocated() {
    return objects.size();
}

size_t MemoryManager::getObjectCount() {
    return objects.size();
}

// Runtime initialization
void Runtime::initialize() {
    if (initialized) return;
    
    // Initialize built-in types
    NoneType = &PyNone::getStaticType();
    BoolType = &PyBool::getStaticType();
    IntType = &PyInt::getStaticType();
    FloatType = &PyFloat::getStaticType();
    StringType = &PyString::getStaticType();
    ListType = &PyList::getStaticType();
    DictType = &PyDict::getStaticType();
    FunctionType = &PyFunction::getStaticType();
    
    // Initialize built-in objects
    NoneObject = PyNone::getInstance();
    TrueObject = PyBool::getInstance(true);
    FalseObject = PyBool::getInstance(false);
    
    // Set up type methods
    setupTypeMethods();
    
    initialized = true;
}

void Runtime::shutdown() {
    if (!initialized) return;
    
    // Clean up
    collectGarbage();
    
    initialized = false;
}

void Runtime::setupTypeMethods() {
    // None type methods
    NoneType->setReprFunc([](const PyObject* obj) -> std::string {
        return "None";
    });
    
    // Bool type methods
    BoolType->setReprFunc([](const PyObject* obj) -> std::string {
        auto bool_obj = static_cast<const PyBool*>(obj);
        return bool_obj->getValue() ? "True" : "False";
    });
    
    // Int type methods
    IntType->setReprFunc([](const PyObject* obj) -> std::string {
        auto int_obj = static_cast<const PyInt*>(obj);
        return std::to_string(int_obj->getValue());
    });
    
    // Float type methods
    FloatType->setReprFunc([](const PyObject* obj) -> std::string {
        auto float_obj = static_cast<const PyFloat*>(obj);
        return std::to_string(float_obj->getValue());
    });
    
    // String type methods
    StringType->setReprFunc([](const PyObject* obj) -> std::string {
        auto str_obj = static_cast<const PyString*>(obj);
        return "\"" + str_obj->getValue() + "\"";
    });
    
    StringType->setGetitemFunc([](PyObject* obj, PyObject* key) -> PyObject* {
        auto str_obj = static_cast<PyString*>(obj);
        if (key->isInt()) {
            auto int_key = static_cast<PyInt*>(key);
            int64_t index = int_key->getValue();
            if (index >= 0 && index < static_cast<int64_t>(str_obj->length())) {
                char c = str_obj->getValue()[index];
                return PyString::create(std::string(1, c));
            }
        }
        return nullptr;
    });
    
    // List type methods
    ListType->setReprFunc([](const PyObject* obj) -> std::string {
        auto list_obj = static_cast<const PyList*>(obj);
        std::string result = "[";
        for (size_t i = 0; i < list_obj->size(); ++i) {
            if (i > 0) result += ", ";
            result += list_obj->get(i)->repr();
        }
        result += "]";
        return result;
    });
    
    ListType->setGetitemFunc([](PyObject* obj, PyObject* key) -> PyObject* {
        auto list_obj = static_cast<PyList*>(obj);
        if (key->isInt()) {
            auto int_key = static_cast<PyInt*>(key);
            int64_t index = int_key->getValue();
            if (index >= 0 && index < static_cast<int64_t>(list_obj->size())) {
                auto item = list_obj->get(index);
                item->ref();
                return item;
            }
        }
        return nullptr;
    });
    
    ListType->setSetitemFunc([](PyObject* obj, PyObject* key, PyObject* value) -> void {
        auto list_obj = static_cast<PyList*>(obj);
        if (key->isInt()) {
            auto int_key = static_cast<PyInt*>(key);
            int64_t index = int_key->getValue();
            if (index >= 0 && index < static_cast<int64_t>(list_obj->size())) {
                list_obj->set(index, value);
            }
        }
    });
    
    // Dict type methods
    DictType->setReprFunc([](const PyObject* obj) -> std::string {
        auto dict_obj = static_cast<const PyDict*>(obj);
        std::string result = "{";
        bool first = true;
        for (const auto& [key, value] : dict_obj->getItems()) {
            if (!first) result += ", ";
            result += "\"" + key + "\": " + value->repr();
            first = false;
        }
        result += "}";
        return result;
    });
    
    DictType->setGetitemFunc([](PyObject* obj, PyObject* key) -> PyObject* {
        auto dict_obj = static_cast<PyDict*>(obj);
        if (key->isString()) {
            auto str_key = static_cast<PyString*>(key);
            auto value = dict_obj->get(str_key->getValue());
            if (value) {
                value->ref();
                return value;
            }
        }
        return nullptr;
    });
    
    DictType->setSetitemFunc([](PyObject* obj, PyObject* key, PyObject* value) -> void {
        auto dict_obj = static_cast<PyDict*>(obj);
        if (key->isString()) {
            auto str_key = static_cast<PyString*>(key);
            dict_obj->set(str_key->getValue(), value);
        }
    });
    
    // Function type methods
    FunctionType->setReprFunc([](const PyObject* obj) -> std::string {
        auto func_obj = static_cast<const PyFunction*>(obj);
        return "<function " + func_obj->getName() + ">";
    });
    
    FunctionType->setCallFunc([](PyObject* obj, const std::vector<PyObject*>& args) -> PyObject* {
        auto func_obj = static_cast<PyFunction*>(obj);
        // This would call the actual compiled function
        // For now, return None
        Runtime::NoneObject->ref();
        return Runtime::NoneObject;
    });
}

// Built-in functions
namespace builtin {

PyObject* print_func(const std::vector<PyObject*>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << args[i]->repr();
    }
    std::cout << std::endl;
    
    Runtime::NoneObject->ref();
    return Runtime::NoneObject;
}

PyObject* len_func(const std::vector<PyObject*>& args) {
    if (args.size() != 1) {
        throw PyException("len() takes exactly one argument");
    }
    
    PyObject* obj = args[0];
    int64_t length = 0;
    
    if (obj->isString()) {
        length = static_cast<PyString*>(obj)->length();
    } else if (obj->isList()) {
        length = static_cast<PyList*>(obj)->size();
    } else if (obj->isDict()) {
        length = static_cast<PyDict*>(obj)->size();
    } else {
        throw PyException("object of type " + obj->getType()->getName() + " has no len()");
    }
    
    return PyInt::create(length);
}

PyObject* range_func(const std::vector<PyObject*>& args) {
    if (args.size() == 0 || args.size() > 3) {
        throw PyException("range() takes 1 to 3 arguments");
    }
    
    int64_t start = 0, stop = 0, step = 1;
    
    if (args.size() == 1) {
        if (!args[0]->isInt()) {
            throw PyException("range() argument must be integer");
        }
        stop = static_cast<PyInt*>(args[0])->getValue();
    } else if (args.size() == 2) {
        if (!args[0]->isInt() || !args[1]->isInt()) {
            throw PyException("range() arguments must be integers");
        }
        start = static_cast<PyInt*>(args[0])->getValue();
        stop = static_cast<PyInt*>(args[1])->getValue();
    } else if (args.size() == 3) {
        if (!args[0]->isInt() || !args[1]->isInt() || !args[2]->isInt()) {
            throw PyException("range() arguments must be integers");
        }
        start = static_cast<PyInt*>(args[0])->getValue();
        stop = static_cast<PyInt*>(args[1])->getValue();
        step = static_cast<PyInt*>(args[2])->getValue();
    }
    
    // Create a list representing the range
    auto list = PyList::create();
    for (int64_t i = start; (step > 0) ? i < stop : i > stop; i += step) {
        list->append(PyInt::create(i));
    }
    
    return list;
}

PyObject* type_func(const std::vector<PyObject*>& args) {
    if (args.size() != 1) {
        throw PyException("type() takes exactly one argument");
    }
    
    // Return the type object
    // For simplicity, return the type name as a string
    auto type_name = PyString::create(args[0]->getType()->getName());
    return type_name;
}

} // namespace builtin

} // namespace pyplusplus