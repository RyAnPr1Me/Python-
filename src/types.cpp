#include "types.h"
#include <algorithm>

namespace pyplusplus {

TypeSystem::TypeSystem() {
    unknown_type = std::make_shared<UnknownType>();
    any_type = std::make_shared<AnyType>();
    none_type = std::make_shared<NoneType>();
    bool_type = std::make_shared<BoolType>();
    int_type = std::make_shared<IntType>();
    float_type = std::make_shared<FloatType>();
    string_type = std::make_shared<StringType>();
}

std::shared_ptr<ListType> TypeSystem::createListType(std::shared_ptr<Type> element_type) {
    return std::make_shared<ListType>(std::move(element_type));
}

std::shared_ptr<DictType> TypeSystem::createDictType(std::shared_ptr<Type> key_type, std::shared_ptr<Type> value_type) {
    return std::make_shared<DictType>(std::move(key_type), std::move(value_type));
}

std::shared_ptr<FunctionType> TypeSystem::createFunctionType(
    std::vector<std::shared_ptr<Type>> parameter_types, 
    std::shared_ptr<Type> return_type, 
    bool is_variadic) {
    return std::make_shared<FunctionType>(std::move(parameter_types), std::move(return_type), is_variadic);
}

std::shared_ptr<UnionType> TypeSystem::createUnionType(std::vector<std::shared_ptr<Type>> types) {
    // Remove duplicates and sort for consistency
    std::vector<std::shared_ptr<Type>> unique_types;
    for (auto& type : types) {
        bool found = false;
        for (auto& existing : unique_types) {
            if (type->equals(*existing)) {
                found = true;
                break;
            }
        }
        if (!found) {
            unique_types.push_back(std::move(type));
        }
    }
    
    if (unique_types.size() == 1) {
        return unique_types[0]; // Single type, no union needed
    }
    
    return std::make_shared<UnionType>(std::move(unique_types));
}

std::shared_ptr<TypeVariable> TypeSystem::createTypeVariable(const std::string& name) {
    return std::make_shared<TypeVariable>(name, any_type, none_type);
}

std::shared_ptr<ClassType> TypeSystem::createClass(const std::string& name) {
    auto class_type = std::make_shared<ClassType>(name);
    classes[name] = class_type;
    return class_type;
}

std::shared_ptr<ClassType> TypeSystem::getClass(const std::string& name) const {
    auto it = classes.find(name);
    return it != classes.end() ? it->second : nullptr;
}

bool TypeSystem::isSubtype(const Type& subtype, const Type& supertype) {
    if (subtype.equals(supertype)) return true;
    if (supertype.getKind() == Type::Kind::ANY) return true;
    if (subtype.getKind() == Type::Kind::NONE) return true;
    
    // Numeric promotion
    if (subtype.getKind() == Type::Kind::INT && supertype.getKind() == Type::Kind::FLOAT) {
        return true;
    }
    
    // Union types: subtype if any member is subtype
    if (subtype.getKind() == Type::Kind::UNION) {
        const auto& union_type = static_cast<const UnionType&>(subtype);
        for (const auto& type : union_type.getTypes()) {
            if (isSubtype(*type, supertype)) {
                return true;
            }
        }
        return false;
    }
    
    // Union supertypes: subtype if subtype of all members
    if (supertype.getKind() == Type::Kind::UNION) {
        const auto& union_type = static_cast<const UnionType&>(supertype);
        for (const auto& type : union_type.getTypes()) {
            if (!isSubtype(subtype, *type)) {
                return false;
            }
        }
        return true;
    }
    
    // Class inheritance (simplified)
    if (subtype.getKind() == Type::Kind::CLASS && supertype.getKind() == Type::Kind::CLASS) {
        const auto& sub_class = static_cast<const ClassType&>(subtype);
        const auto& super_class = static_cast<const ClassType&>(supertype);
        return sub_class.getName() == super_class.getName(); // Simplified: no inheritance
    }
    
    return false;
}

std::shared_ptr<Type> TypeSystem::join(const Type& type1, const Type& type2) {
    if (type1.equals(type2)) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){});
    }
    
    if (type1.getKind() == Type::Kind::ANY) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){});
    }
    if (type2.getKind() == Type::Kind::ANY) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type2), [](Type*){});
    }
    
    if (isSubtype(type1, type2)) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type2), [](Type*){});
    }
    if (isSubtype(type2, type1)) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){});
    }
    
    // Numeric join: int + float = float
    if ((type1.getKind() == Type::Kind::INT && type2.getKind() == Type::Kind::FLOAT) ||
        (type1.getKind() == Type::Kind::FLOAT && type2.getKind() == Type::Kind::INT)) {
        return float_type;
    }
    
    // Otherwise create union
    std::vector<std::shared_ptr<Type>> types;
    types.push_back(std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){}));
    types.push_back(std::shared_ptr<Type>(const_cast<Type*>(&type2), [](Type*){}));
    return createUnionType(std::move(types));
}

std::shared_ptr<Type> TypeSystem::meet(const Type& type1, const Type& type2) {
    if (type1.equals(type2)) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){});
    }
    
    if (type1.getKind() == Type::Kind::ANY) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type2), [](Type*){});
    }
    if (type2.getKind() == Type::Kind::ANY) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){});
    }
    
    if (isSubtype(type1, type2)) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type1), [](Type*){});
    }
    if (isSubtype(type2, type1)) {
        return std::shared_ptr<Type>(const_cast<Type*>(&type2), [](Type*){});
    }
    
    // Numeric meet: int + float = int
    if ((type1.getKind() == Type::Kind::INT && type2.getKind() == Type::Kind::FLOAT) ||
        (type1.getKind() == Type::Kind::FLOAT && type2.getKind() == Type::Kind::INT)) {
        return int_type;
    }
    
    // No meet, return unknown
    return unknown_type;
}

} // namespace pyplusplus