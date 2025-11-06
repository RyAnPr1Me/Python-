#include "stdlib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <cstring>

namespace pyplusplus {

// BuiltinRegistry implementation
BuiltinRegistry& BuiltinRegistry::getInstance() {
    static BuiltinRegistry instance;
    return instance;
}

BuiltinRegistry::BuiltinRegistry() {
    initializeBuiltins();
}

void BuiltinRegistry::initializeBuiltins() {
    // Register all built-in functions
    registerBuiltin("print", StdLib::io::print_func);
    registerBuiltin("len", StdLib::string::len_func);
    registerBuiltin("range", StdLib::iteration::range_func);
    registerBuiltin("type", StdLib::types::type_func);
    registerBuiltin("int", StdLib::types::int_func);
    registerBuiltin("float", StdLib::types::float_func);
    registerBuiltin("str", StdLib::types::str_func);
    registerBuiltin("bool", StdLib::types::bool_func);
    registerBuiltin("abs", StdLib::math::abs_func);
    registerBuiltin("round", StdLib::math::round_func);
    registerBuiltin("min", StdLib::math::min_func);
    registerBuiltin("max", StdLib::math::max_func);
    registerBuiltin("sum", StdLib::math::sum_func);
    registerBuiltin("pow", StdLib::math::pow_func);
    registerBuiltin("list", StdLib::list_ops::list_func);
    registerBuiltin("dict", StdLib::dict_ops::dict_func);
    registerBuiltin("input", StdLib::io::input_func);
    registerBuiltin("open", StdLib::io::open_func);
    registerBuiltin("exit", StdLib::system::exit_func);
    registerBuiltin("enumerate", StdLib::iteration::enumerate_func);
    registerBuiltin("zip", StdLib::iteration::zip_func);
    registerBuiltin("map", StdLib::iteration::map_func);
    registerBuiltin("filter", StdLib::iteration::filter_func);
    registerBuiltin("isinstance", StdLib::types::isinstance_func);
    registerBuiltin("raise", StdLib::exceptions::raise_func);
    registerBuiltin("assert", StdLib::exceptions::assert_func);
}

void BuiltinRegistry::registerBuiltin(const std::string& name, 
                                       std::function<PyObject*(const std::vector<PyObject*>&)> func) {
    builtins[name] = std::move(func);
}

PyObject* BuiltinRegistry::callBuiltin(const std::string& name, const std::vector<PyObject*>& args) {
    auto it = builtins.find(name);
    if (it != builtins.end()) {
        return it->second(args);
    }
    return nullptr;
}

bool BuiltinRegistry::hasBuiltin(const std::string& name) const {
    return builtins.find(name) != builtins.end();
}

std::vector<std::string> BuiltinRegistry::getBuiltinNames() const {
    std::vector<std::string> names;
    for (const auto& [name, func] : builtins) {
        names.push_back(name);
    }
    return names;
}

// Math module implementations
namespace StdLib::math {

PyObject* abs_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "abs")) return nullptr;
    
    PyObject* arg = args[0];
    
    if (arg->isInt()) {
        auto int_arg = static_cast<PyInt*>(arg);
        int64_t value = int_arg->getValue();
        return PyInt::create(std::abs(value));
    } else if (arg->isFloat()) {
        auto float_arg = static_cast<PyFloat*>(arg);
        double value = float_arg->getValue();
        return PyFloat::create(std::abs(value));
    } else {
        throw PyException("abs() argument must be int or float");
    }
}

PyObject* round_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, 2, "round")) return nullptr;
    
    PyObject* arg = args[0];
    int decimals = 0;
    
    if (args.size() == 2) {
        if (!utils::checkArgType(args[1], PyType::Kind::INT, "round", 1)) return nullptr;
        decimals = static_cast<PyInt*>(args[1])->getValue();
    }
    
    if (arg->isInt()) {
        arg->ref();
        return arg;
    } else if (arg->isFloat()) {
        auto float_arg = static_cast<PyFloat*>(arg);
        double value = float_arg->getValue();
        
        if (decimals == 0) {
            return PyInt::create(static_cast<int64_t>(std::round(value)));
        } else {
            double multiplier = std::pow(10, decimals);
            return PyFloat::create(std::round(value * multiplier) / multiplier);
        }
    } else {
        throw PyException("round() argument must be int or float");
    }
}

PyObject* min_func(const std::vector<PyObject*>& args) {
    if (args.empty()) {
        throw PyException("min() requires at least one argument");
    }
    
    PyObject* min_obj = args[0];
    
    for (size_t i = 1; i < args.size(); ++i) {
        // For simplicity, just compare string representations
        if (args[i]->repr() < min_obj->repr()) {
            min_obj = args[i];
        }
    }
    
    min_obj->ref();
    return min_obj;
}

PyObject* max_func(const std::vector<PyObject*>& args) {
    if (args.empty()) {
        throw PyException("max() requires at least one argument");
    }
    
    PyObject* max_obj = args[0];
    
    for (size_t i = 1; i < args.size(); ++i) {
        // For simplicity, just compare string representations
        if (args[i]->repr() > max_obj->repr()) {
            max_obj = args[i];
        }
    }
    
    max_obj->ref();
    return max_obj;
}

PyObject* sum_func(const std::vector<PyObject*>& args) {
    if (args.empty()) {
        return PyInt::create(0);
    }
    
    int64_t int_sum = 0;
    double float_sum = 0.0;
    bool has_float = false;
    
    for (PyObject* arg : args) {
        if (arg->isInt()) {
            auto int_arg = static_cast<PyInt*>(arg);
            int_sum += int_arg->getValue();
            if (has_float) {
                float_sum += int_arg->getValue();
            }
        } else if (arg->isFloat()) {
            auto float_arg = static_cast<PyFloat*>(arg);
            if (!has_float) {
                has_float = true;
                float_sum = int_sum;
            }
            float_sum += float_arg->getValue();
        } else {
            throw PyException("sum() arguments must be int or float");
        }
    }
    
    if (has_float) {
        return PyFloat::create(float_sum);
    } else {
        return PyInt::create(int_sum);
    }
}

PyObject* pow_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 2, "pow")) return nullptr;
    
    if (!utils::checkArgType(args[0], PyType::Kind::INT, "pow", 0) ||
        !utils::checkArgType(args[1], PyType::Kind::INT, "pow", 1)) {
        return nullptr;
    }
    
    auto base = static_cast<PyInt*>(args[0])->getValue();
    auto exp = static_cast<PyInt*>(args[1])->getValue();
    
    if (exp < 0) {
        return PyFloat::create(std::pow(static_cast<double>(base), static_cast<double>(exp)));
    } else {
        return PyInt::create(static_cast<int64_t>(std::pow(base, exp)));
    }
}

} // namespace StdLib::math

// String operations implementations
namespace StdLib::string {

PyObject* len_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "len")) return nullptr;
    
    PyObject* arg = args[0];
    
    if (arg->isString()) {
        auto str_arg = static_cast<PyString*>(arg);
        return PyInt::create(static_cast<int64_t>(str_arg->length()));
    } else if (arg->isList()) {
        auto list_arg = static_cast<PyList*>(arg);
        return PyInt::create(static_cast<int64_t>(list_arg->size()));
    } else if (arg->isDict()) {
        auto dict_arg = static_cast<PyDict*>(arg);
        return PyInt::create(static_cast<int64_t>(dict_arg->size()));
    } else {
        throw PyException("object of type " + arg->getType()->getName() + " has no len()");
    }
}

PyObject* str_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "str")) return nullptr;
    
    return PyString::create(args[0]->repr());
}

PyObject* upper_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "upper")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::STRING, "upper", 0)) return nullptr;
    
    auto str_arg = static_cast<PyString*>(args[0]);
    std::string result = str_arg->getValue();
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    
    return PyString::create(result);
}

PyObject* lower_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "lower")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::STRING, "lower", 0)) return nullptr;
    
    auto str_arg = static_cast<PyString*>(args[0]);
    std::string result = str_arg->getValue();
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    
    return PyString::create(result);
}

PyObject* strip_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "strip")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::STRING, "strip", 0)) return nullptr;
    
    auto str_arg = static_cast<PyString*>(args[0]);
    std::string result = str_arg->getValue();
    
    // Trim leading whitespace
    size_t start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return PyString::create("");
    }
    
    // Trim trailing whitespace
    size_t end = result.find_last_not_of(" \t\n\r");
    result = result.substr(start, end - start + 1);
    
    return PyString::create(result);
}

PyObject* split_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, 2, "split")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::STRING, "split", 0)) return nullptr;
    
    auto str_arg = static_cast<PyString*>(args[0]);
    std::string delimiter = " ";
    
    if (args.size() == 2) {
        if (!utils::checkArgType(args[1], PyType::Kind::STRING, "split", 1)) return nullptr;
        delimiter = static_cast<PyString*>(args[1])->getValue();
    }
    
    std::string str = str_arg->getValue();
    auto result = PyList::create();
    
    size_t start = 0;
    size_t pos = str.find(delimiter);
    
    while (pos != std::string::npos) {
        result->append(PyString::create(str.substr(start, pos - start)));
        start = pos + delimiter.length();
        pos = str.find(delimiter, start);
    }
    
    result->append(PyString::create(str.substr(start)));
    
    return result;
}

PyObject* join_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 2, "join")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::STRING, "join", 0)) return nullptr;
    if (!utils::checkArgType(args[1], PyType::Kind::LIST, "join", 1)) return nullptr;
    
    auto separator = static_cast<PyString*>(args[0])->getValue();
    auto list_arg = static_cast<PyList*>(args[1]);
    
    std::string result;
    for (size_t i = 0; i < list_arg->size(); ++i) {
        if (i > 0) result += separator;
        
        auto item = list_arg->get(i);
        if (!item->isString()) {
            throw PyException("join() argument must be a list of strings");
        }
        result += static_cast<PyString*>(item)->getValue();
    }
    
    return PyString::create(result);
}

} // namespace StdLib::string

// List operations implementations
namespace StdLib::list_ops {

PyObject* list_func(const std::vector<PyObject*>& args) {
    if (args.empty()) {
        return PyList::create();
    }
    
    if (args.size() == 1 && args[0]->isList()) {
        args[0]->ref();
        return args[0];
    }
    
    auto result = PyList::create();
    for (PyObject* arg : args) {
        result->append(arg);
    }
    
    return result;
}

PyObject* append_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 2, "append")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::LIST, "append", 0)) return nullptr;
    
    auto list_arg = static_cast<PyList*>(args[0]);
    list_arg->append(args[1]);
    
    Runtime::NoneObject->ref();
    return Runtime::NoneObject;
}

PyObject* extend_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 2, "extend")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::LIST, "extend", 0)) return nullptr;
    if (!utils::checkArgType(args[1], PyType::Kind::LIST, "extend", 1)) return nullptr;
    
    auto list_arg = static_cast<PyList*>(args[0]);
    auto other_arg = static_cast<PyList*>(args[1]);
    
    for (size_t i = 0; i < other_arg->size(); ++i) {
        list_arg->append(other_arg->get(i));
    }
    
    Runtime::NoneObject->ref();
    return Runtime::NoneObject;
}

} // namespace StdLib::list_ops

// Dictionary operations implementations
namespace StdLib::dict_ops {

PyObject* dict_func(const std::vector<PyObject*>& args) {
    if (args.empty()) {
        return PyDict::create();
    }
    
    if (args.size() == 1 && args[0]->isDict()) {
        args[0]->ref();
        return args[0];
    }
    
    auto result = PyDict::create();
    
    if (args.size() == 1 && args[0]->isList()) {
        auto list_arg = static_cast<PyList*>(args[0]);
        for (size_t i = 0; i < list_arg->size(); ++i) {
            auto item = list_arg->get(i);
            if (item->isList()) {
                auto pair = static_cast<PyList*>(item);
                if (pair->size() == 2) {
                    auto key = pair->get(0);
                    auto value = pair->get(1);
                    if (key->isString()) {
                        result->set(static_cast<PyString*>(key)->getValue(), value);
                    }
                }
            }
        }
    }
    
    return result;
}

PyObject* keys_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "keys")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::DICT, "keys", 0)) return nullptr;
    
    auto dict_arg = static_cast<PyDict*>(args[0]);
    auto result = PyList::create();
    
    for (const auto& [key, value] : dict_arg->getItems()) {
        result->append(PyString::create(key));
    }
    
    return result;
}

PyObject* values_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "values")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::DICT, "values", 0)) return nullptr;
    
    auto dict_arg = static_cast<PyDict*>(args[0]);
    auto result = PyList::create();
    
    for (const auto& [key, value] : dict_arg->getItems()) {
        result->append(value);
    }
    
    return result;
}

} // namespace StdLib::dict_ops

// I/O operations implementations
namespace StdLib::io {

PyObject* print_func(const std::vector<PyObject*>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << args[i]->repr();
    }
    std::cout << std::endl;
    
    Runtime::NoneObject->ref();
    return Runtime::NoneObject;
}

PyObject* input_func(const std::vector<PyObject*>& args) {
    if (args.size() > 1) {
        throw PyException("input() takes at most 1 argument");
    }
    
    if (args.size() == 1) {
        if (!utils::checkArgType(args[0], PyType::Kind::STRING, "input", 0)) return nullptr;
        std::cout << static_cast<PyString*>(args[0])->getValue();
    }
    
    std::string line;
    std::getline(std::cin, line);
    
    return PyString::create(line);
}

PyObject* open_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 2, "open")) return nullptr;
    if (!utils::checkArgType(args[0], PyType::Kind::STRING, "open", 0)) return nullptr;
    if (!utils::checkArgType(args[1], PyType::Kind::STRING, "open", 1)) return nullptr;
    
    auto filename = static_cast<PyString*>(args[0])->getValue();
    auto mode = static_cast<PyString*>(args[1])->getValue();
    
    return PyFile::create(filename, mode);
}

} // namespace StdLib::io

// System operations implementations
namespace StdLib::system {

PyObject* exit_func(const std::vector<PyObject*>& args) {
    int exit_code = 0;
    
    if (!args.empty()) {
        if (!utils::checkArgType(args[0], PyType::Kind::INT, "exit", 0)) return nullptr;
        exit_code = static_cast<int>(static_cast<PyInt*>(args[0])->getValue());
    }
    
    std::exit(exit_code);
    return nullptr; // Never reached
}

} // namespace StdLib::system

// Type conversion implementations
namespace StdLib::types {

PyObject* int_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "int")) return nullptr;
    
    PyObject* arg = args[0];
    
    if (arg->isInt()) {
        arg->ref();
        return arg;
    } else if (arg->isFloat()) {
        auto float_arg = static_cast<PyFloat*>(arg);
        return PyInt::create(static_cast<int64_t>(float_arg->getValue()));
    } else if (arg->isString()) {
        auto str_arg = static_cast<PyString*>(arg);
        try {
            return PyInt::create(static_cast<int64_t>(std::stoll(str_arg->getValue())));
        } catch (const std::exception&) {
            throw PyException("invalid literal for int() with base 10: '" + str_arg->getValue() + "'");
        }
    } else if (arg->isBool()) {
        auto bool_arg = static_cast<PyBool*>(arg);
        return PyInt::create(bool_arg->getValue() ? 1 : 0);
    } else {
        throw PyException("int() argument must be a string, a bytes-like object or a number, not '" + 
                         arg->getType()->getName() + "'");
    }
}

PyObject* float_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "float")) return nullptr;
    
    PyObject* arg = args[0];
    
    if (arg->isFloat()) {
        arg->ref();
        return arg;
    } else if (arg->isInt()) {
        auto int_arg = static_cast<PyInt*>(arg);
        return PyFloat::create(static_cast<double>(int_arg->getValue()));
    } else if (arg->isString()) {
        auto str_arg = static_cast<PyString*>(arg);
        try {
            return PyFloat::create(std::stod(str_arg->getValue()));
        } catch (const std::exception&) {
            throw PyException("could not convert string to float: '" + str_arg->getValue() + "'");
        }
    } else if (arg->isBool()) {
        auto bool_arg = static_cast<PyBool*>(arg);
        return PyFloat::create(bool_arg->getValue() ? 1.0 : 0.0);
    } else {
        throw PyException("float() argument must be a string or a number, not '" + 
                         arg->getType()->getName() + "'");
    }
}

PyObject* bool_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "bool")) return nullptr;
    
    PyObject* arg = args[0];
    
    if (arg->isNone()) {
        return Runtime::FalseObject;
    } else if (arg->isBool()) {
        arg->ref();
        return arg;
    } else if (arg->isInt()) {
        auto int_arg = static_cast<PyInt*>(arg);
        return PyBool::getInstance(int_arg->getValue() != 0);
    } else if (arg->isFloat()) {
        auto float_arg = static_cast<PyFloat*>(arg);
        return PyBool::getInstance(float_arg->getValue() != 0.0);
    } else if (arg->isString()) {
        auto str_arg = static_cast<PyString*>(arg);
        return PyBool::getInstance(!str_arg->getValue().empty());
    } else if (arg->isList()) {
        auto list_arg = static_cast<PyList*>(arg);
        return PyBool::getInstance(list_arg->size() > 0);
    } else if (arg->isDict()) {
        auto dict_arg = static_cast<PyDict*>(arg);
        return PyBool::getInstance(dict_arg->size() > 0);
    } else {
        return Runtime::TrueObject;
    }
}

PyObject* type_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "type")) return nullptr;
    
    return PyString::create(args[0]->getType()->getName());
}

PyObject* isinstance_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 2, "isinstance")) return nullptr;
    
    PyObject* obj = args[0];
    PyObject* type_obj = args[1];
    
    if (!type_obj->isString()) {
        throw PyException("isinstance() arg 2 must be a type or tuple of types");
    }
    
    auto type_name = static_cast<PyString*>(type_obj)->getValue();
    bool result = obj->getType()->getName() == type_name;
    
    return PyBool::getInstance(result);
}

} // namespace StdLib::types

// Iteration implementations
namespace StdLib::iteration {

PyObject* range_func(const std::vector<PyObject*>& args) {
    if (args.empty() || args.size() > 3) {
        throw PyException("range() takes 1 to 3 arguments");
    }
    
    int64_t start = 0, stop = 0, step = 1;
    
    if (args.size() == 1) {
        if (!utils::checkArgType(args[0], PyType::Kind::INT, "range", 0)) return nullptr;
        stop = static_cast<PyInt*>(args[0])->getValue();
    } else if (args.size() == 2) {
        if (!utils::checkArgType(args[0], PyType::Kind::INT, "range", 0)) return nullptr;
        if (!utils::checkArgType(args[1], PyType::Kind::INT, "range", 1)) return nullptr;
        start = static_cast<PyInt*>(args[0])->getValue();
        stop = static_cast<PyInt*>(args[1])->getValue();
    } else if (args.size() == 3) {
        if (!utils::checkArgType(args[0], PyType::Kind::INT, "range", 0)) return nullptr;
        if (!utils::checkArgType(args[1], PyType::Kind::INT, "range", 1)) return nullptr;
        if (!utils::checkArgType(args[2], PyType::Kind::INT, "range", 2)) return nullptr;
        start = static_cast<PyInt*>(args[0])->getValue();
        stop = static_cast<PyInt*>(args[1])->getValue();
        step = static_cast<PyInt*>(args[2])->getValue();
    }
    
    auto list = PyList::create();
    for (int64_t i = start; (step > 0) ? i < stop : i > stop; i += step) {
        list->append(PyInt::create(i));
    }
    
    return list;
}

} // namespace StdLib::iteration

// Exception implementations
namespace StdLib::exceptions {

PyObject* raise_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, "raise")) return nullptr;
    
    std::string message = args[0]->repr();
    throw PyException(message);
    
    return nullptr; // Never reached
}

PyObject* assert_func(const std::vector<PyObject*>& args) {
    if (!utils::checkArgCount(args, 1, 2, "assert")) return nullptr;
    
    bool condition = false;
    
    if (args[0]->isBool()) {
        condition = static_cast<PyBool*>(args[0])->getValue();
    } else if (args[0]->isInt()) {
        condition = static_cast<PyInt*>(args[0])->getValue() != 0;
    } else if (args[0]->isFloat()) {
        condition = static_cast<PyFloat*>(args[0])->getValue() != 0.0;
    } else if (args[0]->isString()) {
        condition = !static_cast<PyString*>(args[0])->getValue().empty();
    } else if (args[0]->isList()) {
        condition = static_cast<PyList*>(args[0])->size() > 0;
    } else if (args[0]->isDict()) {
        condition = static_cast<PyDict*>(args[0])->size() > 0;
    } else {
        condition = true; // Non-empty objects are truthy
    }
    
    if (!condition) {
        std::string message = "Assertion failed";
        if (args.size() == 2) {
            message += ": " + args[1]->repr();
        }
        throw PyException(message);
    }
    
    Runtime::NoneObject->ref();
    return Runtime::NoneObject;
}

} // namespace StdLib::exceptions

// PyFile implementation
PyFile::PyFile(const std::string& filename, const std::string& mode) 
    : PyObject(&getStaticType()), filename(filename), mode(mode) {
    file_handle = std::fopen(filename.c_str(), mode.c_str());
    if (!file_handle) {
        throw PyException("Cannot open file: " + filename);
    }
}

PyFile::~PyFile() {
    if (file_handle) {
        std::fclose(file_handle);
    }
}

PyType& PyFile::getStaticType() {
    static PyType type("file", PyType::Kind::CLASS, sizeof(PyFile));
    return type;
}

PyFile* PyFile::create(const std::string& filename, const std::string& mode) {
    return new PyFile(filename, mode);
}

// Utility functions implementations
namespace utils {

bool checkArgCount(const std::vector<PyObject*>& args, size_t expected, const std::string& func_name) {
    if (args.size() != expected) {
        throw PyException(argCountError(func_name, expected, args.size()));
    }
    return true;
}

bool checkArgCount(const std::vector<PyObject*>& args, size_t min, size_t max, const std::string& func_name) {
    if (args.size() < min || args.size() > max) {
        throw PyException(argCountError(func_name, min, max, args.size()));
    }
    return true;
}

bool checkArgType(PyObject* arg, PyType* expected_type, const std::string& func_name, size_t arg_index) {
    if (!arg->isInstance(expected_type)) {
        throw PyException(typeError(func_name, arg_index, expected_type, arg->getType()));
    }
    return true;
}

bool checkArgType(PyObject* arg, PyType::Kind expected_kind, const std::string& func_name, size_t arg_index) {
    switch (expected_kind) {
        case PyType::Kind::BOOL: return arg->isBool();
        case PyType::Kind::INT: return arg->isInt();
        case PyType::Kind::FLOAT: return arg->isFloat();
        case PyType::Kind::STRING: return arg->isString();
        case PyType::Kind::LIST: return arg->isList();
        case PyType::Kind::DICT: return arg->isDict();
        case PyType::Kind::FUNCTION: return arg->isFunction();
        case PyType::Kind::NONE: return arg->isNone();
        default: return false;
    }
}

std::string typeError(const std::string& func_name, size_t arg_index, 
                      PyType* expected_type, PyType* actual_type) {
    return func_name + "() argument " + std::to_string(arg_index) + 
           " must be " + expected_type->getName() + ", not " + actual_type->getName();
}

std::string argCountError(const std::string& func_name, size_t expected, size_t actual) {
    return func_name + "() takes exactly " + std::to_string(expected) + 
           " argument" + (expected == 1 ? "" : "s") + " (" + std::to_string(actual) + " given)";
}

std::string argCountError(const std::string& func_name, size_t min, size_t max, size_t actual) {
    return func_name + "() takes from " + std::to_string(min) + " to " + std::to_string(max) + 
           " arguments (" + std::to_string(actual) + " given)";
}

} // namespace utils

// StdLib initialization
void StdLib::initialize() {
    BuiltinRegistry::getInstance();
    ModuleRegistry::getInstance();
}

} // namespace pyplusplus