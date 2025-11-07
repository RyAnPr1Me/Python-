#include "runtime.h"
#include "import_system.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace pyplusplus {

// C API implementation
extern "C" {

// Object management
void* Py_IncRef(void* obj) {
    if (obj) {
        auto* py_obj = static_cast<PyObject*>(obj);
        py_obj->ref();
    }
    return obj;
}

void Py_DecRef(void* obj) {
    if (obj) {
        auto* py_obj = static_cast<PyObject*>(obj);
        py_obj->unref();
    }
}

void* PyObject_Malloc(size_t size) {
    return MemoryManager::allocate(size);
}

void PyObject_Free(void* ptr) {
    MemoryManager::deallocate(ptr);
}

// Type creation
void* PyLong_FromLong(long value) {
    return PyInt::create(static_cast<int64_t>(value));
}

void* PyFloat_FromDouble(double value) {
    return PyFloat::create(value);
}

void* PyUnicode_FromString(const char* str) {
    return PyString::create(std::string(str));
}

const char* PyUnicode_AsUTF8(void* obj) {
    if (auto* str = dynamic_cast<PyString*>(static_cast<PyObject*>(obj))) {
        return str->getValue().c_str();
    }
    return nullptr;
}

long PyLong_AsLong(void* obj) {
    if (auto* int_obj = dynamic_cast<PyInt*>(static_cast<PyObject*>(obj))) {
        return static_cast<long>(int_obj->getValue());
    }
    return 0;
}

double PyFloat_AsDouble(void* obj) {
    if (auto* float_obj = dynamic_cast<PyFloat*>(static_cast<PyObject*>(obj))) {
        return float_obj->getValue();
    }
    return 0.0;
}

// Operations
void* PyNumber_Add(void* left, void* right) {
    auto* left_obj = static_cast<PyObject*>(left);
    auto* right_obj = static_cast<PyObject*>(right);
    
    // Handle different numeric types
    if (auto* left_int = dynamic_cast<PyInt*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            return PyInt::create(left_int->getValue() + right_int->getValue());
        }
        if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
            return PyFloat::create(static_cast<double>(left_int->getValue()) + right_float->getValue());
        }
    }
    
    if (auto* left_float = dynamic_cast<PyFloat*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            return PyFloat::create(left_float->getValue() + static_cast<double>(right_int->getValue()));
        }
        if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
            return PyFloat::create(left_float->getValue() + right_float->getValue());
        }
    }
    
    // Handle string concatenation
    if (auto* left_str = dynamic_cast<PyString*>(left_obj)) {
        if (auto* right_str = dynamic_cast<PyString*>(right_obj)) {
            return PyString::create(left_str->getValue() + right_str->getValue());
        }
    }
    
    return nullptr;
}

void* PyNumber_Subtract(void* left, void* right) {
    auto* left_obj = static_cast<PyObject*>(left);
    auto* right_obj = static_cast<PyObject*>(right);
    
    if (auto* left_int = dynamic_cast<PyInt*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            return PyInt::create(left_int->getValue() - right_int->getValue());
        }
        if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
            return PyFloat::create(static_cast<double>(left_int->getValue()) - right_float->getValue());
        }
    }
    
    if (auto* left_float = dynamic_cast<PyFloat*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            return PyFloat::create(left_float->getValue() - static_cast<double>(right_int->getValue()));
        }
        if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
            return PyFloat::create(left_float->getValue() - right_float->getValue());
        }
    }
    
    return nullptr;
}

void* PyNumber_Multiply(void* left, void* right) {
    auto* left_obj = static_cast<PyObject*>(left);
    auto* right_obj = static_cast<PyObject*>(right);
    
    if (auto* left_int = dynamic_cast<PyInt*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            return PyInt::create(left_int->getValue() * right_int->getValue());
        }
        if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
            return PyFloat::create(static_cast<double>(left_int->getValue()) * right_float->getValue());
        }
    }
    
    if (auto* left_float = dynamic_cast<PyFloat*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            return PyFloat::create(left_float->getValue() * static_cast<double>(right_int->getValue()));
        }
        if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
            return PyFloat::create(left_float->getValue() * right_float->getValue());
        }
    }
    
    // Handle string repetition
    if (auto* left_str = dynamic_cast<PyString*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            std::string result;
            for (int64_t i = 0; i < right_int->getValue(); ++i) {
                result += left_str->getValue();
            }
            return PyString::create(result);
        }
    }
    
    return nullptr;
}

void* PyNumber_TrueDivide(void* left, void* right) {
    auto* left_obj = static_cast<PyObject*>(left);
    auto* right_obj = static_cast<PyObject*>(right);
    
    double left_val = 0.0, right_val = 0.0;
    
    if (auto* left_int = dynamic_cast<PyInt*>(left_obj)) {
        left_val = static_cast<double>(left_int->getValue());
    } else if (auto* left_float = dynamic_cast<PyFloat*>(left_obj)) {
        left_val = left_float->getValue();
    }
    
    if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
        right_val = static_cast<double>(right_int->getValue());
    } else if (auto* right_float = dynamic_cast<PyFloat*>(right_obj)) {
        right_val = right_float->getValue();
    }
    
    if (right_val != 0.0) {
        return PyFloat::create(left_val / right_val);
    }
    
    return nullptr;
}

void* PyObject_RichCompare(void* left, void* right, int opid) {
    auto* left_obj = static_cast<PyObject*>(left);
    auto* right_obj = static_cast<PyObject*>(right);
    
    bool result = false;
    
    // Handle numeric comparisons
    if (auto* left_int = dynamic_cast<PyInt*>(left_obj)) {
        if (auto* right_int = dynamic_cast<PyInt*>(right_obj)) {
            switch (opid) {
                case 0: result = left_int->getValue() < right_int->getValue(); break;  // Py_LT
                case 1: result = left_int->getValue() <= right_int->getValue(); break; // Py_LE
                case 2: result = left_int->getValue() == right_int->getValue(); break; // Py_EQ
                case 3: result = left_int->getValue() != right_int->getValue(); break; // Py_NE
                case 4: result = left_int->getValue() > right_int->getValue(); break;  // Py_GT
                case 5: result = left_int->getValue() >= right_int->getValue(); break; // Py_GE
            }
        }
    }
    
    // Handle string comparisons
    if (auto* left_str = dynamic_cast<PyString*>(left_obj)) {
        if (auto* right_str = dynamic_cast<PyString*>(right_obj)) {
            int cmp = left_str->getValue().compare(right_str->getValue());
            switch (opid) {
                case 0: result = cmp < 0; break;   // Py_LT
                case 1: result = cmp <= 0; break;  // Py_LE
                case 2: result = cmp == 0; break;  // Py_EQ
                case 3: result = cmp != 0; break;  // Py_NE
                case 4: result = cmp > 0; break;   // Py_GT
                case 5: result = cmp >= 0; break;  // Py_GE
            }
        }
    }
    
    return PyBool::getInstance(result);
}

// Function calls
void* PyObject_Call(void* callable, void* args, void* kwargs) {
    auto* func = dynamic_cast<PyFunction*>(static_cast<PyObject*>(callable));
    if (!func) {
        return nullptr;
    }
    
    // Extract arguments from tuple
    std::vector<PyObject*> arg_list;
    if (args) {
        auto* args_tuple = dynamic_cast<PyList*>(static_cast<PyObject*>(args));
        if (args_tuple) {
            for (size_t i = 0; i < args_tuple->size(); ++i) {
                arg_list.push_back(args_tuple->get(i));
            }
        }
    }
    
    // Call the function
    return AOTRuntime::executeFunction(func, arg_list);
}

void* PyObject_CallFunction(void* callable, const char* format, ...) {
    // Simple implementation for common cases
    auto* func = dynamic_cast<PyFunction*>(static_cast<PyObject*>(callable));
    if (!func) {
        return nullptr;
    }
    
    std::vector<PyObject*> args;
    
    if (format && *format) {
        va_list vargs;
        va_start(vargs, format);
        
        for (const char* p = format; *p; ++p) {
            switch (*p) {
                case 'i': {
                    int value = va_arg(vargs, int);
                    args.push_back(PyInt::create(value));
                    break;
                }
                case 'l': {
                    long value = va_arg(vargs, long);
                    args.push_back(PyInt::create(value));
                    break;
                }
                case 'd': {
                    double value = va_arg(vargs, double);
                    args.push_back(PyFloat::create(value));
                    break;
                }
                case 's': {
                    char* value = va_arg(vargs, char*);
                    args.push_back(PyString::create(std::string(value)));
                    break;
                }
                case 'O': {
                    void* value = va_arg(vargs, void*);
                    args.push_back(static_cast<PyObject*>(value));
                    break;
                }
            }
        }
        
        va_end(vargs);
    }
    
    return AOTRuntime::executeFunction(func, args);
}

// Exception handling
void* PyErr_Occurred() {
    auto* thread_state = PyThreadState::getCurrent();
    return thread_state ? thread_state->getException() : nullptr;
}

void PyErr_SetString(void* exception_type, const char* message) {
    auto* exc_obj = static_cast<PyObject*>(exception_type);
    auto* msg_obj = PyString::create(std::string(message));
    
    // Create exception instance
    auto exception = std::make_unique<PyException>(message, exc_obj);
    
    auto* thread_state = PyThreadState::getCurrent();
    if (thread_state) {
        thread_state->setException(exc_obj);
    }
}

void PyErr_Clear() {
    auto* thread_state = PyThreadState::getCurrent();
    if (thread_state) {
        thread_state->clearException();
    }
}

// Module system
void* PyImport_ImportModule(const char* name) {
    auto& import_system = ImportSystem::getInstance();
    return import_system.import(std::string(name));
}

void* PyModule_GetDict(void* module) {
    auto* mod = dynamic_cast<PyModule*>(static_cast<PyObject*>(module));
    return mod ? mod->getGlobals() : nullptr;
}

void* PyModule_GetName(void* module) {
    auto* mod = dynamic_cast<PyModule*>(static_cast<PyObject*>(module));
    if (mod) {
        return PyString::create(mod->getName());
    }
    return nullptr;
}

int PyModule_AddObject(void* module, const char* name, void* value) {
    auto* mod = dynamic_cast<PyModule*>(static_cast<PyObject*>(module));
    if (mod && value) {
        mod->setGlobal(std::string(name), static_cast<PyObject*>(value));
        return 0;
    }
    return -1;
}

// Runtime initialization
void Py_Initialize() {
    Runtime::initialize();
    AOTRuntime::initialize();
    ImportSystem::initialize();
}

void Py_Finalize() {
    ImportSystem::shutdown();
    AOTRuntime::shutdown();
    Runtime::shutdown();
}

int Py_IsInitialized() {
    return Runtime::isInitialized() ? 1 : 0;
}

// List operations
void* PyList_New(size_t size) {
    auto* list = PyList::create();
    list->getItems().reserve(size);
    return list;
}

size_t PyList_Size(void* list) {
    auto* lst = dynamic_cast<PyList*>(static_cast<PyObject*>(list));
    return lst ? lst->size() : 0;
}

void* PyList_GetItem(void* list, size_t index) {
    auto* lst = dynamic_cast<PyList*>(static_cast<PyObject*>(list));
    return lst ? lst->get(index) : nullptr;
}

int PyList_SetItem(void* list, size_t index, void* item) {
    auto* lst = dynamic_cast<PyList*>(static_cast<PyObject*>(list));
    if (lst && item) {
        lst->set(index, static_cast<PyObject*>(item));
        return 0;
    }
    return -1;
}

int PyList_Append(void* list, void* item) {
    auto* lst = dynamic_cast<PyList*>(static_cast<PyObject*>(list));
    if (lst && item) {
        lst->append(static_cast<PyObject*>(item));
        return 0;
    }
    return -1;
}

// Dictionary operations
void* PyDict_New() {
    return PyDict::create();
}

size_t PyDict_Size(void* dict) {
    auto* d = dynamic_cast<PyDict*>(static_cast<PyObject*>(dict));
    return d ? d->size() : 0;
}

void* PyDict_GetItemString(void* dict, const char* key) {
    auto* d = dynamic_cast<PyDict*>(static_cast<PyObject*>(dict));
    return d ? d->get(std::string(key)) : nullptr;
}

int PyDict_SetItemString(void* dict, const char* key, void* item) {
    auto* d = dynamic_cast<PyDict*>(static_cast<PyObject*>(dict));
    if (d && item) {
        d->set(std::string(key), static_cast<PyObject*>(item));
        return 0;
    }
    return -1;
}

int PyDict_DelItemString(void* dict, const char* key) {
    auto* d = dynamic_cast<PyDict*>(static_cast<PyObject*>(dict));
    if (d) {
        // Implementation would need to support deletion
        return 0;
    }
    return -1;
}

// Boolean operations
void* PyBool_FromLong(long value) {
    return PyBool::getInstance(value != 0);
}

int PyBool_Check(void* obj) {
    return dynamic_cast<PyBool*>(static_cast<PyObject*>(obj)) != nullptr;
}

// Type checking
int PyLong_Check(void* obj) {
    return dynamic_cast<PyInt*>(static_cast<PyObject*>(obj)) != nullptr;
}

int PyFloat_Check(void* obj) {
    return dynamic_cast<PyFloat*>(static_cast<PyObject*>(obj)) != nullptr;
}

int PyUnicode_Check(void* obj) {
    return dynamic_cast<PyString*>(static_cast<PyObject*>(obj)) != nullptr;
}

int PyList_Check(void* obj) {
    return dynamic_cast<PyList*>(static_cast<PyObject*>(obj)) != nullptr;
}

int PyDict_Check(void* obj) {
    return dynamic_cast<PyDict*>(static_cast<PyObject*>(obj)) != nullptr;
}

int PyFunction_Check(void* obj) {
    return dynamic_cast<PyFunction*>(static_cast<PyObject*>(obj)) != nullptr;
}

int PyModule_Check(void* obj) {
    return dynamic_cast<PyModule*>(static_cast<PyObject*>(obj)) != nullptr;
}

// Reference counting utilities
void Py_XINCREF(void* obj) {
    Py_IncRef(obj);
}

void Py_XDECREF(void* obj) {
    Py_DecRef(obj);
}

// String operations
Py_ssize_t PyUnicode_GetLength(void* obj) {
    auto* str = dynamic_cast<PyString*>(static_cast<PyObject*>(obj));
    return str ? static_cast<Py_ssize_t>(str->getValue().length()) : 0;
}

// Attribute access
void* PyObject_GetAttrString(void* obj, const char* attr_name) {
    auto* py_obj = static_cast<PyObject*>(obj);
    if (!py_obj) {
        return nullptr;
    }
    
    // This would need proper attribute resolution implementation
    // For now, return None
    return Runtime::NoneObject;
}

int PyObject_SetAttrString(void* obj, const char* attr_name, void* value) {
    auto* py_obj = static_cast<PyObject*>(obj);
    if (!py_obj) {
        return -1;
    }
    
    // This would need proper attribute setting implementation
    return 0;
}

// Method calls
void* PyObject_CallMethod(void* obj, const char* method_name, const char* format, ...) {
    // Get method object first
    void* method = PyObject_GetAttrString(obj, method_name);
    if (!method) {
        return nullptr;
    }
    
    // Call the method
    void* result = nullptr;
    if (format) {
        va_list vargs;
        va_start(vargs, format);
        result = PyObject_CallFunction(method, format, vargs);
        va_end(vargs);
    } else {
        result = PyObject_Call(method, nullptr, nullptr);
    }
    
    Py_DecRef(method);
    return result;
}

// Sequence protocol
Py_ssize_t PySequence_Size(void* seq) {
    if (auto* list = dynamic_cast<PyList*>(static_cast<PyObject*>(seq))) {
        return static_cast<Py_ssize_t>(list->size());
    }
    if (auto* dict = dynamic_cast<PyDict*>(static_cast<PyObject*>(seq))) {
        return static_cast<Py_ssize_t>(dict->size());
    }
    return -1;
}

void* PySequence_GetItem(void* seq, Py_ssize_t index) {
    if (auto* list = dynamic_cast<PyList*>(static_cast<PyObject*>(seq))) {
        return list->get(static_cast<size_t>(index));
    }
    return nullptr;
}

// Mapping protocol
void* PyMapping_GetItemString(void* mapping, const char* key) {
    return PyDict_GetItemString(mapping, key);
}

int PyMapping_SetItemString(void* mapping, const char* key, void* value) {
    return PyDict_SetItemString(mapping, key, value);
}

// Iterator protocol
void* PyObject_GetIter(void* obj) {
    // This would need proper iterator implementation
    return nullptr;
}

void* PyIter_Next(void* iter) {
    // This would need proper iterator implementation
    return nullptr;
}

} // extern "C"

} // namespace pyplusplus