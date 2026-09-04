#ifndef CODEGEN_OBJECTMODELTABLE_H
#define CODEGEN_OBJECTMODELTABLE_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "semantics/typed-ast/Classes.h"

static const int WORD_SIZE = 4;
static const int ATTR_START = 3;
static const int METHOD_START = 0;

static const int DISP_TABLE_OFF = 8;

struct ObjectModel {
    struct Feature {
        Type owner;
        std::string name;
    };
    std::vector<Feature> all_attrs;
    std::vector<Feature> all_methods;
    std::unordered_map<std::string, int> attr_name_to_off;
    std::unordered_map<std::string, int> method_name_to_off;
};

class ObjectModelTable {
public:
    ObjectModelTable(Classes *ast);

    int get_attr_offset(Type type, const std::string &attr) const;
    int get_method_offset(Type type, const std::string &method) const;

    ObjectModel::Feature find_method(Type type, std::string method_name);

    std::vector<ObjectModel::Feature> get_all_attrs(Type type) const;
    std::vector<ObjectModel::Feature> get_all_methods(Type type) const;

    bool has_method(Type t, std::string method) const;
    bool has_attr(Type t, std::string attr) const;

    bool is_nop_initializable(Type t) const;
    std::vector<Type> get_nop_initializable() const;

private:
    std::vector<ObjectModel> models;
    std::unordered_set<Type> nop_initializable;
    std::vector<Type> nop_initializables;
};

#endif // CODEGEN_OBJECTMODELTABLE_H
