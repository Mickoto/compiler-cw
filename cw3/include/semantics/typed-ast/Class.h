#ifndef SEMANTICS_TYPED_AST_CLASS_H_
#define SEMANTICS_TYPED_AST_CLASS_H_
#include "Attributes.h"
#include "Methods.h"

#include <string>

class Class {
private:
    std::string name_;
    Type parent;
    bool builtin, def_initializable;
    Attributes attributes;
    Methods methods;

public:
    Class(std::string name, bool builtin, bool def_initializable) : name_(name), builtin(builtin), def_initializable(def_initializable) {}
    Class(const Class &) = delete;
    Class(Class &&) = default;

    const Type &get_parent() const { return parent; }
    const std::string &get_name() const { return name_; }
    bool is_builtin() const { return builtin; }
    bool is_default_initializable() const { return def_initializable; }

    void set_parent(Type parent) {
        this->parent = parent;
    }

    Attributes *get_attributes() { return &attributes; }
    Methods *get_methods() { return &methods; }
};

#endif
