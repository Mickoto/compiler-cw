#ifndef SEMANTICS_TYPED_AST_CLASS_H_
#define SEMANTICS_TYPED_AST_CLASS_H_
#include "Attributes.h"
#include "Methods.h"

#include <memory>
#include <string>

class Class {
private:
    std::string name_;
    Type parent;
    Attributes attributes;
    // Methods methods;

public:
    Class(std::string name) : name_(name) {}

    const Type &get_parent() const { return parent; }
    const std::string &get_name() const { return name_; }

    void set_parent(Type parent) {
        this->parent = parent;
    }

    Attributes *get_attributes() { return &attributes; }
    // Methods *get_methods() { return &methods; }
};

#endif
