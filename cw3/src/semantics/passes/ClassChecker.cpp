#include "semantics/passes/ClassChecker.h"

using std::vector;

bool TypeTree::contains(const std::string& classname) const {
    return classname == "Object" || name_to_idx.count(classname);
}

Type TypeTree::add_class(const std::string& classname) {
    name_to_idx.insert({classname, classes.size()});
    ClassInfo info;
    info.name = classname;
    classes.push_back(info);

    return classes.size() - 1;
}

void TypeTree::set_super(Type base, Type super) {
    classes[base].super = super;
}

Type TypeTree::insert(const std::string& classname, Type super) {
    Type ret = add_class(classname);
    set_super(ret, super);
    return ret;
}

vector<Type> TypeTree::get_classes() const { 
    // NOTE: Extremely scuffed
    std::vector<Type> ret;
    for (int i = 0; i < classes.size(); i++) {
        ret.push_back(i);
    }
    return ret;
}

Type TypeTree::from_name(const std::string& classname) const {
    if (classname == "Object") return root_type;
    return name_to_idx.at(classname);
}

ClassInfo& TypeTree::getInfo(Type t) {
    return classes[t];
}

Type TypeTree::getParent(Type t) const {
    return classes[t].super;
}

bool TypeTree::isSuper(Type t, Type sup) {
    while (t != root_type) {
        if (t == sup) return true;
        t = getParent(t);
    }
    return t == sup;
}

Type TypeTree::lub(Type t1, Type t2) {
    std::unordered_set<Type> visited;
    while(t1 != root_type) {
        visited.insert(t1);
        t1 = getParent(t1);
    }
    while(t2 != root_type) {
        if (visited.count(t2)) break;
        t2 = getParent(t2);
    }
    return t2;
}

std::expected<TypeTree, vector<std::string> > TypeTreeBuilder::build(CoolParser *parser) {
    type_tree = TypeTree();
    type_tree.insert("Int", type_tree.root_type);
    type_tree.insert("Bool", type_tree.root_type);
    type_tree.insert("String", type_tree.root_type);
    type_tree.insert("IO", type_tree.root_type);
    visitProgram(parser->program());

    for (auto pair : supers) {
        if (!type_tree.contains(pair.second)) {
            errors.push_back(type_tree.getInfo(pair.first).name + " inherits from undefined class " + pair.second);
        }
        else {
            type_tree.set_super(pair.first, type_tree.from_name(pair.second));
        }
    }

    if (errors.empty())
        return std::move(type_tree);
    else
        return std::unexpected(errors);
}

std::any TypeTreeBuilder::visitClass(CoolParser::ClassContext *ctx) {
    std::string name = ctx->classname->getText();
    if (type_tree.contains(name)) {
        errors.push_back(name + " redefined!");
    }

    Type t = type_tree.add_class(name);
    if (ctx->inherit)
        supers.push_back({t, ctx->inherit->getText()});
    else
        type_tree.set_super(t, type_tree.root_type);

    return std::any {};
}

std::string print_inheritance_loops_error(vector<vector<std::string>> inheritance_loops) {
    std::stringstream eout;
    eout << "Detected " << inheritance_loops.size()
         << " loops in the type hierarchy:" << std::endl;
    for (int i = 0; i < inheritance_loops.size(); ++i) {
        eout << i + 1 << ") ";
        auto &loop = inheritance_loops[i];
        for (std::string name : loop) {
            eout << name << " <- ";
        }
        eout << std::endl;
    }

    return eout.str();
}

vector<std::string> checkLoops(TypeTree &tree) {
    vector<std::string> errors;

    std::unordered_set<Type> cycle_classes;
    vector<vector<std::string>> cycles;
    for (Type t : tree.get_classes()) {
        if (cycle_classes.count(t)) continue;
        Type curr = tree.getParent(t);
        std::unordered_set<Type> visited;
        visited.insert(t);
        while (curr != tree.root_type) {
            if (visited.count(curr)) {
                Type end = curr;
                vector<std::string> cycle;
                do {
                    cycle.push_back(tree.getInfo(curr).name);
                    cycle_classes.insert(curr);
                    curr = tree.getParent(curr);
                }
                while (curr != end);
                cycles.push_back(cycle);
                break;
            }
            visited.insert(curr);
            curr = tree.getParent(curr);
        }
    }

    if (!cycles.empty()) {
        errors.push_back(print_inheritance_loops_error(cycles));
    }
    return errors;
}
