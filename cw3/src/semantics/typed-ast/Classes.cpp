#include "Classes.h"

#include <unordered_set>

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

vector<Type> Classes::get_classes() const { 
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

Type Classes::get_parent(Type t) const {
    return classes[t].get_parent();
}

bool Classes::is_super(Type t, Type sup) {
    while (t != no_type) {
        if (t == sup) return true;
        t = get_parent(t);
    }
    return t == sup;
}

Type Classes::lub(Type t1, Type t2) {
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
