#include "Classes.h"

#include <unordered_set>
#include <cassert>

using std::vector;

bool Classes::contains(const std::string& classname) const {
    return name_to_idx.count(classname);
}

Type Classes::add(const std::string& classname) {
    name_to_idx.insert({classname, classes.size()});
    classes.push_back(classname);

    return classes.size() - 1;
}

Type Classes::insert(const std::string& classname, Type super) {
    Type ret = add(classname);
    classes[ret].set_parent(super);
    return ret;
}

vector<Type> Classes::get_types() const {
    std::vector<Type> ret;
    for (int i = 0; i < classes.size(); i++) {
        ret.push_back(i);
    }
    return ret;
}

Type Classes::from_name(const std::string& classname) const {
    return name_to_idx.at(classname);
}

Class *Classes::get_class(Type t) {
    return &classes[t];
}

std::string Classes::get_name(Type t) const {
    if (is_self_type(t)) {
        return "SELF_TYPE";
    }
    return classes[t].get_name();
}

Type Classes::get_parent(Type t) const {
    assert(t >= 0);
    if (is_self_type(t))
        return t - classes.size();
    return classes[t].get_parent();
}

bool Classes::is_super(Type t, Type sup) const {
    if (t == error_type) return true;
    if (is_self_type(sup))
        return t == sup;

    while (t != no_type) {
        if (t == sup) return true;
        t = get_parent(t);
    }
    return t == sup;
}

Type Classes::lub(Type t1, Type t2) const {
    if (is_self_type(t1))
        t1 = get_parent(t1);
    if (is_self_type(t2))
        t2 = get_parent(t2);
    if (t1 == error_type) return t2;
    if (t2 == error_type) return t1;

    std::unordered_set<Type> visited;
    while(t1 != no_type) {
        visited.insert(t1);
        t1 = get_parent(t1);
    }
    while(t2 != no_type) {
        if (visited.count(t2)) break;
        t2 = get_parent(t2);
    }
    return t2;
}

Type Classes::self_type(Type t) const {
    assert(t >= 0);
    assert(t < classes.size());

    return t + classes.size();
}

bool Classes::is_self_type(Type t) const {
    return t >= (int)classes.size();
}
