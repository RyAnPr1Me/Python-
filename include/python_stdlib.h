#pragma once
#include "types.h"
#include "refcount.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace pyplusplus {

// Python built-in functions
class BuiltinFunctions {
public:
    // Numeric functions
    static Value abs(const Value& x);
    static Value round(const Value& number, const Value& digits = Value(0));
    static Value pow(const Value& base, const Value& exp);
    static Value divmod(const Value& x, const Value& y);
    static Value max(const std::vector<Value>& args);
    static Value min(const std::vector<Value>& args);
    static Value sum(const std::vector<Value>& iterable, const Value& start = Value(0));
    
    // Type conversion functions
    static Value int_func(const Value& x);
    static Value float_func(const Value& x);
    static Value str_func(const Value& obj);
    static Value bool_func(const Value& x);
    static Value list_func(const Value& iterable = Value());
    static Value dict_func(const Value& mapping = Value());
    static Value tuple_func(const Value& iterable = Value());
    static Value set_func(const Value& iterable = Value());
    static Value frozenset_func(const Value& iterable = Value());
    
    // String functions
    static Value len(const Value& obj);
    static Value chr(const Value& i);
    static Value ord(const Value& c);
    static Value hex(const Value& x);
    static Value oct(const Value& x);
    static Value bin(const Value& x);
    static Value ascii(const Value& obj);
    
    // Iterable functions
    static Value all_func(const std::vector<Value>& iterable);
    static Value any_func(const std::vector<Value>& iterable);
    static Value enumerate(const Value& iterable, const Value& start = Value(0));
    static Value range_func(const std::vector<Value>& args);
    static Value zip_func(const std::vector<Value>& iterables);
    static Value map_func(const Value& function, const Value& iterable);
    static Value filter_func(const Value& function, const Value& iterable);
    static Value reversed(const Value& sequence);
    static Value sorted(const Value& iterable, const Value& key = Value(), const Value& reverse = Value(false));
    
    // Object functions
    static Value isinstance(const Value& obj, const Value& classinfo);
    static Value issubclass(const Value& cls, const Value& classinfo);
    static Value hasattr(const Value& obj, const Value& name);
    static Value getattr(const Value& obj, const Value& name, const Value& default_val = Value());
    static Value setattr(const Value& obj, const Value& name, const Value& value);
    static Value delattr(const Value& obj, const Value& name);
    static Value id_func(const Value& obj);
    static Value hash_func(const Value& obj);
    static Value repr(const Value& obj);
    static Value ascii_func(const Value& obj);
    
    // Collection functions
    static Value len_func(const Value& obj);
    static Value slice(const Value& sequence, const Value& start, const Value& stop, const Value& step = Value());
    static Value concat(const std::vector<Value>& sequences);
    
    // I/O functions
    static Value print_func(const std::vector<Value>& args, const Value& sep = Value(" "), const Value& end = Value("\n"));
    static Value input_func(const Value& prompt = Value(""));
    static Value open_func(const Value& file, const Value& mode = Value("r"));
    
    // Reflection functions
    static Value type_func(const Value& obj);
    static Value callable(const Value& obj);
    static Value dir_func(const Value& obj = Value());
    static Value vars_func(const Value& obj = Value());
    static Value globals_func();
    static Value locals_func();
    
    // Compilation functions
    static Value eval_func(const Value& expression, const Value& globals = Value(), const Value& locals = Value());
    static Value exec_func(const Value& object, const Value& globals = Value(), const Value& locals = Value());
    static Value compile_func(const Value& source, const Value& filename, const Value& mode);
    
    // Attribute functions
    static Value getattr_func(const Value& obj, const Value& name, const Value& default_val = Value());
    static Value setattr_func(const Value& obj, const Value& name, const Value& value);
    static Value delattr_func(const Value& obj, const Value& name);
    static Value hasattr_func(const Value& obj, const Value& name);
    
    // Memory functions
    static Value memoryview_func(const Value& obj);
    static Value bytearray_func(const Value& source = Value(), const Value& encoding = Value(), const Value& errors = Value());
    static Value bytes_func(const Value& source = Value(), const Value& encoding = Value(), const Value& errors = Value());
    
    // Math functions
    static Value math_abs(const Value& x);
    static Value math_fabs(const Value& x);
    static Value math_ceil(const Value& x);
    static Value math_floor(const Value& x);
    static Value math_factorial(const Value& x);
    static Value math_fmod(const Value& x, const Value& y);
    static Value math_gcd(const Value& a, const Value& b);
    static Value math_exp(const Value& x);
    static Value math_log(const Value& x, const Value& base = Value());
    static Value math_log2(const Value& x);
    static Value math_log10(const Value& x);
    static Value math_pow(const Value& x, const Value& y);
    static Value math_sqrt(const Value& x);
    static Value math_sin(const Value& x);
    static Value math_cos(const Value& x);
    static Value math_tan(const Value& x);
    static Value math_asin(const Value& x);
    static Value math_acos(const Value& x);
    static Value math_atan(const Value& x);
    static Value math_atan2(const Value& y, const Value& x);
    static Value math_sinh(const Value& x);
    static Value math_cosh(const Value& x);
    static Value math_tanh(const Value& x);
    static Value math_asinh(const Value& x);
    static Value math_acosh(const Value& x);
    static Value math_atanh(const Value& x);
    static Value math_degrees(const Value& x);
    static Value math_radians(const Value& x);
    static Value math_hypot(const std::vector<Value>& coordinates);
    
    // Iterator functions
    static Value iter_func(const Value& obj);
    static Value next_func(const Value& iterator, const Value& default_val = Value());
    static Value stop_iteration();
    
    // Class and function creation
    static Value classmethod_func(const Value& function);
    static Value staticmethod_func(const Value& function);
    static Value property_func(const Value& fget = Value(), const Value& fset = Value(), const Value& fdel = Value());
    static Value super_func(const Value& type = Value(), const Value& obj_or_none = Value());
    
    // Module functions
    static Value import_func(const Value& name, const Value& globals = Value(), const Value& locals = Value(), 
                          const Value& fromlist = Value(), const Value& level = Value());
    static Value reload_func(const Value& module);
    
    // Debug functions
    static Value breakpoint_func();
    static Value assert_func(const Value& condition, const Value& message = Value());
};

// Python standard library modules
class StdLibModules {
public:
    // Math module
    static RefPtr<RefCountedMap> getMathModule();
    
    // String module
    static RefPtr<RefCountedMap> getStringModule();
    
    // List module
    static RefPtr<RefCountedMap> getListModule();
    
    // Dict module
    static RefPtr<RefCountedMap> getDictModule();
    
    // Set module
    static RefPtr<RefCountedMap> getSetModule();
    
    // File I/O module
    static RefPtr<RefCountedMap> getIOModule();
    
    // OS module
    static RefPtr<RefCountedMap> getOSModule();
    
    // System module
    static RefPtr<RefCountedMap> getSystemModule();
    
    // JSON module
    static RefPtr<RefCountedMap> getJSONModule();
    
    // Re module
    static RefPtr<RefCountedMap> getRegexModule();
    
    // Time module
    static RefPtr<RefCountedMap> getTimeModule();
    
    // Random module
    static RefPtr<RefCountedMap> getRandomModule();
    
    // Collections module
    static RefPtr<RefCountedMap> getCollectionsModule();
    
    // Itertools module
    static RefPtr<RefCountedMap> getItertoolsModule();
    
    // Functools module
    static RefPtr<RefCountedMap> getFunctoolsModule();
    
    // Operator module
    static RefPtr<RefCountedMap> getOperatorModule();
    
    // All modules
    static std::unordered_map<std::string, RefPtr<RefCountedMap>> getAllModules();
};

// Python data model implementation
class PythonDataModel {
public:
    // Basic object protocol
    static Value __str__(const Value& obj);
    static Value __repr__(const Value& obj);
    static Value __bytes__(const Value& obj);
    static Value __format__(const Value& obj, const Value& format_spec);
    
    // Comparison protocol
    static Value __lt__(const Value& self, const Value& other);
    static Value __le__(const Value& self, const Value& other);
    static Value __eq__(const Value& self, const Value& other);
    static Value __ne__(const Value& self, const Value& other);
    static Value __gt__(const Value& self, const Value& other);
    static Value __ge__(const Value& self, const Value& other);
    
    // Numeric protocol
    static Value __add__(const Value& self, const Value& other);
    static Value __sub__(const Value& self, const Value& other);
    static Value __mul__(const Value& self, const Value& other);
    static Value __truediv__(const Value& self, const Value& other);
    static Value __floordiv__(const Value& self, const Value& other);
    static Value __mod__(const Value& self, const Value& other);
    static Value __pow__(const Value& self, const Value& other);
    static Value __neg__(const Value& self);
    static Value __pos__(const Value& self);
    static Value __abs__(const Value& self);
    
    // Container protocol
    static Value __len__(const Value& obj);
    static Value __getitem__(const Value& obj, const Value& key);
    static Value __setitem__(const Value& obj, const Value& key, const Value& value);
    static Value __delitem__(const Value& obj, const Value& key);
    static Value __contains__(const Value& obj, const Value& item);
    static Value __iter__(const Value& obj);
    static Value __next__(const Value& iterator);
    
    // Callable protocol
    static Value __call__(const Value& obj, const std::vector<Value>& args, const std::unordered_map<std::string, Value>& kwargs);
    
    // Attribute protocol
    static Value __getattr__(const Value& obj, const Value& name);
    static Value __setattr__(const Value& obj, const Value& name, const Value& value);
    static Value __delattr__(const Value& obj, const Value& name);
    static Value __dir__(const Value& obj);
    
    // Descriptor protocol
    static Value __get__(const Value& descriptor, const Value& obj, const Value& objtype);
    static Value __set__(const Value& descriptor, const Value& obj, const Value& value);
    static Value __delete__(const Value& descriptor, const Value& obj);
    
    // Context manager protocol
    static Value __enter__(const Value& obj);
    static Value __exit__(const Value& obj, const Value& exc_type, const Value& exc_value, const Value& traceback);
    
    // Async protocol
    static Value __await__(const Value& obj);
    static Value __aiter__(const Value& obj);
    static Value __anext__(const Value& async_iterator);
    static Value __aenter__(const Value& obj);
    static Value __aexit__(const Value& obj, const Value& exc_type, const Value& exc_value, const Value& traceback);
    
    // Pattern matching (Python 3.10+)
    static Value match_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
    static Value match_value_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
    static Value match_sequence_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
    static Value match_mapping_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
    static Value match_class_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
    static Value match_as_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
    static Value match_or_pattern(const Value& pattern, const Value& subject, std::unordered_map<std::string, Value>& bindings);
};

// Exception hierarchy
class PythonExceptions {
public:
    // Base exception class
    class Exception : public RefCounted {
    private:
        RefPtr<RefCountedString> message;
        RefPtr<RefCountedVector> args;
        
    public:
        Exception(const std::string& msg = "") : message(RefCountUtils::makeRef<RefCountedString>(msg)) {}
        
        RefPtr<RefCountedString> getMessage() const { return message; }
        void setMessage(const std::string& msg) { message = RefCountUtils::makeRef<RefCountedString>(msg); }
        
        RefPtr<RefCountedVector> getArgs() const { return args; }
        void setArgs(const RefPtr<RefCountedVector>& new_args) { args = new_args; }
        
        void dispose() override {}
    };
    
    // Specific exception types
    class TypeError : public Exception {
    public:
        TypeError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class ValueError : public Exception {
    public:
        ValueError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class IndexError : public Exception {
    public:
        IndexError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class KeyError : public Exception {
    public:
        KeyError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class AttributeError : public Exception {
    public:
        AttributeError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class NameError : public Exception {
    public:
        NameError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class ZeroDivisionError : public Exception {
    public:
        ZeroDivisionError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class OverflowError : public Exception {
    public:
        OverflowError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class MemoryError : public Exception {
    public:
        MemoryError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class FileNotFoundError : public Exception {
    public:
        FileNotFoundError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class StopIteration : public Exception {
    public:
        StopIteration(const std::string& msg = "") : Exception(msg) {}
    };
    
    class GeneratorExit : public Exception {
    public:
        GeneratorExit(const std::string& msg = "") : Exception(msg) {}
    };
    
    class SystemExit : public Exception {
    private:
        int exit_code;
        
    public:
        SystemExit(int code = 0) : exit_code(code) {}
        int getExitCode() const { return exit_code; }
    };
    
    class KeyboardInterrupt : public Exception {
    public:
        KeyboardInterrupt(const std::string& msg = "") : Exception(msg) {}
    };
    
    class AssertionError : public Exception {
    public:
        AssertionError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class ImportError : public Exception {
    public:
        ImportError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class ModuleNotFoundError : public ImportError {
    public:
        ModuleNotFoundError(const std::string& msg = "") : ImportError(msg) {}
    };
    
    class SyntaxError : public Exception {
    public:
        SyntaxError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class IndentationError : public SyntaxError {
    public:
        IndentationError(const std::string& msg = "") : SyntaxError(msg) {}
    };
    
    class RuntimeError : public Exception {
    public:
        RuntimeError(const std::string& msg = "") : Exception(msg) {}
    };
    
    class NotImplementedError : public RuntimeError {
    public:
        NotImplementedError(const std::string& msg = "") : RuntimeError(msg) {}
    };
    
    // Exception factory functions
    static RefPtr<Exception> createException(const std::string& type, const std::string& message = "");
    static RefPtr<TypeError> createTypeError(const std::string& message = "");
    static RefPtr<ValueError> createValueError(const std::string& message = "");
    static RefPtr<IndexError> createIndexError(const std::string& message = "");
    static RefPtr<KeyError> createKeyError(const std::string& message = "");
    static RefPtr<AttributeError> createAttributeError(const std::string& message = "");
    static RefPtr<NameError> createNameError(const std::string& message = "");
    static RefPtr<ZeroDivisionError> createZeroDivisionError(const std::string& message = "");
    static RefPtr<OverflowError> createOverflowError(const std::string& message = "");
    static RefPtr<MemoryError> createMemoryError(const std::string& message = "");
    static RefPtr<FileNotFoundError> createFileNotFoundError(const std::string& message = "");
    static RefPtr<StopIteration> createStopIteration(const std::string& message = "");
    static RefPtr<AssertionError> createAssertionError(const std::string& message = "");
    static RefPtr<ImportError> createImportError(const std::string& message = "");
    static RefPtr<SyntaxError> createSyntaxError(const std::string& message = "");
    static RefPtr<RuntimeError> createRuntimeError(const std::string& message = "");
    static RefPtr<NotImplementedError> createNotImplementedError(const std::string& message = "");
};

// Python built-in types
class PythonTypes {
public:
    // Object class
    class Object : public RefCounted {
    public:
        virtual RefPtr<RefCountedString> __str__() const;
        virtual RefPtr<RefCountedString> __repr__() const;
        virtual bool __eq__(const Object& other) const;
        virtual int __hash__() const;
        virtual RefPtr<RefCountedString> __class__() const;
        
        void dispose() override {}
    };
    
    // Descriptor base class
    class Descriptor : public Object {
    public:
        virtual Value get(const Value& obj, const Value& objtype) = 0;
        virtual Value set(const Value& obj, const Value& value);
        virtual Value delete_(const Value& obj);
        virtual void set_name(const std::string& name);
        
        void dispose() override {}
    };
    
    // Property descriptor
    class Property : public Descriptor {
    private:
        std::function<Value(const Value&)> getter;
        std::function<void(const Value&, const Value&)> setter;
        std::function<void(const Value&)> deleter;
        std::string name;
        
    public:
        Property(std::function<Value(const Value&)> get_func = nullptr,
                 std::function<void(const Value&, const Value&)> set_func = nullptr,
                 std::function<void(const Value&)> del_func = nullptr);
        
        Value get(const Value& obj, const Value& objtype) override;
        Value set(const Value& obj, const Value& value) override;
        Value delete_(const Value& obj) override;
        void set_name(const std::string& name) override;
        
        void dispose() override {}
    };
    
    // Static method descriptor
    class StaticMethod : public Descriptor {
    private:
        Value function;
        
    public:
        explicit StaticMethod(const Value& func);
        
        Value get(const Value& obj, const Value& objtype) override;
        
        void dispose() override {}
    };
    
    // Class method descriptor
    class ClassMethod : public Descriptor {
    private:
        Value function;
        
    public:
        explicit ClassMethod(const Value& func);
        
        Value get(const Value& obj, const Value& objtype) override;
        
        void dispose() override {}
    };
    
    // Metaclass base
    class Metaclass : public Type {
    public:
        Metaclass(const std::string& name, RefPtr<Type> base_type = nullptr);
        
        virtual Value __call__(const std::vector<Value>& args, 
                               const std::unordered_map<std::string, Value>& kwargs);
        virtual Value __instancecheck__(const Value& obj);
        virtual Value __subclasscheck__(const Value& subclass);
        
        void dispose() override {}
    };
    
    // Singleton metaclass
    class SingletonMetaclass : public Metaclass {
    private:
        static std::unordered_map<std::string, Value> instances;
        
    public:
        SingletonMetaclass();
        
        Value __call__(const std::vector<Value>& args, 
                       const std::unordered_map<std::string, Value>& kwargs) override;
        
        void dispose() override {}
    };
    
    // Type class
    class Type : public Object {
    private:
        RefPtr<RefCountedString> name;
        RefPtr<Type> base;
        
    public:
        Type(const std::string& type_name, RefPtr<Type> base_type = nullptr);
        
        RefPtr<RefCountedString> getName() const { return name; }
        RefPtr<Type> getBase() const { return base; }
        
        bool isSubclassOf(const RefPtr<Type>& other) const;
        RefPtr<Object> newInstance(const std::vector<Value>& args = {});
        
        void dispose() override {}
    };
    
    // Function class
    class Function : public Object {
    private:
        std::string name;
        std::vector<std::string> parameter_names;
        std::function<Value(const std::vector<Value>&)> native_func;
        
    public:
        Function(const std::string& func_name, 
                const std::vector<std::string>& params,
                std::function<Value(const std::vector<Value>&)> func);
        
        Value call(const std::vector<Value>& args = {});
        RefPtr<RefCountedString> getName() const;
        size_t getParameterCount() const;
        
        void dispose() override {}
    };
    
    // Method class
    class Method : public Function {
    private:
        RefPtr<Object> instance;
        
    public:
        Method(const RefPtr<Object>& obj, const std::string& method_name);
        
        Value call(const std::vector<Value>& args = {});
        RefPtr<Object> getInstance() const { return instance; }
        
        void dispose() override {}
    };
    
    // Module class
    class Module : public Object {
    private:
        RefPtr<RefCountedString> name;
        RefPtr<RefCountedMap> attributes;
        
    public:
        Module(const std::string& module_name);
        
        RefPtr<RefCountedString> getName() const { return name; }
        void setAttribute(const std::string& attr_name, const Value& value);
        Value getAttribute(const std::string& attr_name);
        bool hasAttribute(const std::string& attr_name) const;
        
        void dispose() override {}
    };
    
    // Type factory functions
    static RefPtr<Type> getObjectType();
    static RefPtr<Type> getTypeType();
    static RefPtr<Type> getFunctionType();
    static RefPtr<Type> getModuleType();
    static RefPtr<Type> getIntType();
    static RefPtr<Type> getFloatType();
    static RefPtr<Type> getStringType();
    static RefPtr<Type> getListType();
    static RefPtr<Type> getDictType();
    static RefPtr<Type> getTupleType();
    static RefPtr<Type> getSetType();
    static RefPtr<Type> getBoolType();
    static RefPtr<Type> getNoneType();
    
    // Union type (Python 3.10+)
    class UnionType : public Type {
    private:
        std::vector<RefPtr<Type>> union_types;
        
    public:
        UnionType(const std::vector<RefPtr<Type>>& types);
        
        const std::vector<RefPtr<Type>>& getUnionTypes() const { return union_types; }
        bool isInstance(const Value& obj) const;
        RefPtr<RefCountedString> getUnionName() const;
        
        void dispose() override {}
    };
    
    static RefPtr<UnionType> createUnionType(const std::vector<RefPtr<Type>>& types);
};

} // namespace pyplusplus