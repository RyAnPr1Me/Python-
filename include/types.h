#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>

namespace pyplusplus {

// Forward declarations
class Type;
class TypeVariable;
class FunctionType;
class ClassType;
class UnionType;

// Type base class
class Type {
public:
    enum class Kind {
        UNKNOWN,
        ANY,
        NONE,
        BOOL,
        INT,
        FLOAT,
        STRING,
        LIST,
        DICT,
        FUNCTION,
        CLASS,
        UNION,
        TYPE_VARIABLE,
        INTERSECTION,
        PROTOCOL,
        TYPEDDICT,
        LITERAL
    };
    
    virtual ~Type() = default;
    virtual Kind getKind() const = 0;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type& other) const = 0;
    virtual bool isSubtypeOf(const Type& other) const { return equals(other); }
    
    template<typename T>
    bool is() const {
        return getKind() == T::staticKind();
    }
    
    template<typename T>
    const T* as() const {
        return is<T>() ? static_cast<const T*>(this) : nullptr;
    }
};

// Concrete types
class UnknownType : public Type {
public:
    Kind getKind() const override { return Kind::UNKNOWN; }
    static Kind staticKind() { return Kind::UNKNOWN; }
    std::string toString() const override { return "unknown"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::UNKNOWN; }
};

class AnyType : public Type {
public:
    Kind getKind() const override { return Kind::ANY; }
    static Kind staticKind() { return Kind::ANY; }
    std::string toString() const override { return "Any"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::ANY; }
    bool isSubtypeOf(const Type& other) const override { return true; }
};

class NoneType : public Type {
public:
    Kind getKind() const override { return Kind::NONE; }
    static Kind staticKind() { return Kind::NONE; }
    std::string toString() const override { return "None"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::NONE; }
};

class BoolType : public Type {
public:
    Kind getKind() const override { return Kind::BOOL; }
    static Kind staticKind() { return Kind::BOOL; }
    std::string toString() const override { return "bool"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::BOOL; }
};

class IntType : public Type {
public:
    Kind getKind() const override { return Kind::INT; }
    static Kind staticKind() { return Kind::INT; }
    std::string toString() const override { return "int"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::INT; }
    bool isSubtypeOf(const Type& other) const override { return other.getKind() == Kind::FLOAT || equals(other); }
};

class FloatType : public Type {
public:
    Kind getKind() const override { return Kind::FLOAT; }
    static Kind staticKind() { return Kind::FLOAT; }
    std::string toString() const override { return "float"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::FLOAT; }
};

class StringType : public Type {
public:
    Kind getKind() const override { return Kind::STRING; }
    static Kind staticKind() { return Kind::STRING; }
    std::string toString() const override { return "str"; }
    bool equals(const Type& other) const override { return other.getKind() == Kind::STRING; }
};

class ListType : public Type {
private:
    std::shared_ptr<Type> element_type;
    
public:
    explicit ListType(std::shared_ptr<Type> element_type) : element_type(std::move(element_type)) {}
    
    Kind getKind() const override { return Kind::LIST; }
    static Kind staticKind() { return Kind::LIST; }
    std::string toString() const override { return "list[" + element_type->toString() + "]"; }
    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::LIST) return false;
        const auto& other_list = static_cast<const ListType&>(other);
        return element_type->equals(*other_list.element_type);
    }
    
    const Type* getElementType() const { return element_type.get(); }
};

class DictType : public Type {
private:
    std::shared_ptr<Type> key_type;
    std::shared_ptr<Type> value_type;
    
public:
    DictType(std::shared_ptr<Type> key_type, std::shared_ptr<Type> value_type)
        : key_type(std::move(key_type)), value_type(std::move(value_type)) {}
    
    Kind getKind() const override { return Kind::DICT; }
    static Kind staticKind() { return Kind::DICT; }
    std::string toString() const override { 
        return "dict[" + key_type->toString() + ", " + value_type->toString() + "]"; 
    }
    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::DICT) return false;
        const auto& other_dict = static_cast<const DictType&>(other);
        return key_type->equals(*other_dict.key_type) && value_type->equals(*other_dict.value_type);
    }
    
    const Type* getKeyType() const { return key_type.get(); }
    const Type* getValueType() const { return value_type.get(); }
};

class FunctionType : public Type {
private:
    std::vector<std::shared_ptr<Type>> parameter_types;
    std::shared_ptr<Type> return_type;
    bool is_variadic;
    
public:
    FunctionType(std::vector<std::shared_ptr<Type>> parameter_types, 
                 std::shared_ptr<Type> return_type, bool is_variadic = false)
        : parameter_types(std::move(parameter_types)), 
          return_type(std::move(return_type)), 
          is_variadic(is_variadic) {}
    
    Kind getKind() const override { return Kind::FUNCTION; }
    static Kind staticKind() { return Kind::FUNCTION; }
    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < parameter_types.size(); ++i) {
            if (i > 0) result += ", ";
            result += parameter_types[i]->toString();
        }
        if (is_variadic) {
            if (!parameter_types.empty()) result += ", ";
            result += "*args";
        }
        result += ") -> " + return_type->toString();
        return result;
    }
    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::FUNCTION) return false;
        const auto& other_func = static_cast<const FunctionType&>(other);
        if (parameter_types.size() != other_func.parameter_types.size()) return false;
        if (is_variadic != other_func.is_variadic) return false;
        if (!return_type->equals(*other_func.return_type)) return false;
        for (size_t i = 0; i < parameter_types.size(); ++i) {
            if (!parameter_types[i]->equals(*other_func.parameter_types[i])) return false;
        }
        return true;
    }
    
    const std::vector<std::shared_ptr<Type>>& getParameterTypes() const { return parameter_types; }
    const Type* getReturnType() const { return return_type.get(); }
    bool isVariadic() const { return is_variadic; }
};

class ClassType : public Type {
private:
    std::string name;
    std::vector<std::shared_ptr<Type>> base_classes;
    std::unordered_map<std::string, std::shared_ptr<Type>> attributes;
    std::unordered_map<std::string, std::shared_ptr<FunctionType>> methods;
    
public:
    explicit ClassType(const std::string& name) : name(name) {}
    
    Kind getKind() const override { return Kind::CLASS; }
    static Kind staticKind() { return Kind::CLASS; }
    std::string toString() const override { return name; }
    bool equals(const Type& other) const override {
        return other.getKind() == Kind::CLASS && 
               static_cast<const ClassType&>(other).name == name;
    }
    
    const std::string& getName() const { return name; }
    void addBaseClass(std::shared_ptr<Type> base) { base_classes.push_back(std::move(base)); }
    void addAttribute(const std::string& name, std::shared_ptr<Type> type) { 
        attributes[name] = std::move(type); 
    }
    void addMethod(const std::string& name, std::shared_ptr<FunctionType> method) { 
        methods[name] = std::move(method); 
    }
    
    const Type* getAttributeType(const std::string& name) const {
        auto it = attributes.find(name);
        return it != attributes.end() ? it->second.get() : nullptr;
    }
    
    const FunctionType* getMethod(const std::string& name) const {
        auto it = methods.find(name);
        return it != methods.end() ? it->second.get() : nullptr;
    }
};

class UnionType : public Type {
private:
    std::vector<std::shared_ptr<Type>> types;
    
public:
    explicit UnionType(std::vector<std::shared_ptr<Type>> types) : types(std::move(types)) {}
    
    Kind getKind() const override { return Kind::UNION; }
    static Kind staticKind() { return Kind::UNION; }
    std::string toString() const override {
        std::string result = "Union[";
        for (size_t i = 0; i < types.size(); ++i) {
            if (i > 0) result += ", ";
            result += types[i]->toString();
        }
        result += "]";
        return result;
    }
    bool equals(const Type& other) const override {
        if (other.getKind() != Kind::UNION) return false;
        const auto& other_union = static_cast<const UnionType&>(other);
        if (types.size() != other_union.types.size()) return false;
        for (const auto& type : types) {
            bool found = false;
            for (const auto& other_type : other_union.types) {
                if (type->equals(*other_type)) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    
    const std::vector<std::shared_ptr<Type>>& getTypes() const { return types; }
};

class TypeVariable : public Type {
private:
    std::string name;
    std::shared_ptr<Type> upper_bound;
    std::shared_ptr<Type> lower_bound;
    
public:
    explicit TypeVariable(const std::string& name, 
                         std::shared_ptr<Type> upper_bound = nullptr,
                         std::shared_ptr<Type> lower_bound = nullptr)
        : name(name), upper_bound(std::move(upper_bound)), lower_bound(std::move(lower_bound)) {}
    
    Kind getKind() const override { return Kind::TYPE_VARIABLE; }
    static Kind staticKind() { return Kind::TYPE_VARIABLE; }
    std::string toString() const override { return "~" + name; }
    bool equals(const Type& other) const override {
        return other.getKind() == Kind::TYPE_VARIABLE && 
               static_cast<const TypeVariable&>(other).name == name;
    }
    
    const std::string& getName() const { return name; }
    const Type* getUpperBound() const { return upper_bound.get(); }
    const Type* getLowerBound() const { return lower_bound.get(); }
};

// Type system utilities
class TypeSystem {
private:
    std::shared_ptr<UnknownType> unknown_type;
    std::shared_ptr<AnyType> any_type;
    std::shared_ptr<NoneType> none_type;
    std::shared_ptr<BoolType> bool_type;
    std::shared_ptr<IntType> int_type;
    std::shared_ptr<FloatType> float_type;
    std::shared_ptr<StringType> string_type;
    
    std::unordered_map<std::string, std::shared_ptr<ClassType>> classes;
    
public:
    TypeSystem();
    
    // Built-in types
    std::shared_ptr<Type> getUnknownType() const { return unknown_type; }
    std::shared_ptr<Type> getAnyType() const { return any_type; }
    std::shared_ptr<Type> getNoneType() const { return none_type; }
    std::shared_ptr<Type> getBoolType() const { return bool_type; }
    std::shared_ptr<Type> getIntType() const { return int_type; }
    std::shared_ptr<Type> getFloatType() const { return float_type; }
    std::shared_ptr<Type> getStringType() const { return string_type; }
    
    // Type constructors
    std::shared_ptr<ListType> createListType(std::shared_ptr<Type> element_type);
    std::shared_ptr<DictType> createDictType(std::shared_ptr<Type> key_type, std::shared_ptr<Type> value_type);
    std::shared_ptr<FunctionType> createFunctionType(std::vector<std::shared_ptr<Type>> parameter_types, 
                                                      std::shared_ptr<Type> return_type, bool is_variadic = false);
    std::shared_ptr<UnionType> createUnionType(std::vector<std::shared_ptr<Type>> types);
    std::shared_ptr<TypeVariable> createTypeVariable(const std::string& name);
    
    // Class management
    std::shared_ptr<ClassType> createClass(const std::string& name);
    std::shared_ptr<ClassType> getClass(const std::string& name) const;
    
    // Type operations
    bool isSubtype(const Type& subtype, const Type& supertype);
    std::shared_ptr<Type> join(const Type& type1, const Type& type2);
    std::shared_ptr<Type> meet(const Type& type1, const Type& type2);
};

} // namespace pyplusplus