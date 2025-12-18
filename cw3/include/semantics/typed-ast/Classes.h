#ifndef SEMANTICS_TYPED_AST_CLASSES_H_
#define SEMANTICS_TYPED_AST_CLASSES_H_

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Class.h"

class Classes {
private:
    std::unordered_map<std::string, int> name_to_idx;
    std::vector<Class> classes;

public:
    Classes() = default;
    Classes(const Classes &) = delete;
    Classes(Classes &&) = default;
    Classes& operator=(const Classes &) = delete;
    Classes& operator=(Classes &&) = default;

    static const Type no_type = -1;

    bool contains(const std::string& classname) const;
    Type add(const std::string& classname);
    Type insert(const std::string& classname, Type super);

    std::vector<Type> get_classes() const;
    Type from_name(const std::string& classname) const;
    Class *get_class(Type t);
    Type get_parent(Type t) const;
    bool is_super(Type t, Type sup);
    Type lub(Type t1, Type t2);
};

#endif
