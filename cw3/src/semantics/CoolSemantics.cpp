#include "CoolSemantics.h"

#include <expected>
#include <string>
#include <vector>

#include "passes/ClassCollector.h"
#include "passes/FeatureCollector.h"
#include "passes/TypeChecker.h"

using namespace std;

std::vector<std::string> check_loops(Classes &tree);
std::vector<std::string> check_overwrites(Classes &tree);
std::vector<std::string> checkOverwrites(Classes &classes);

// Runs semantic analysis and returns a list of errors, if any.
expected<Classes, vector<string>> CoolSemantics::run() {
    vector<string> errors;
    Classes ast;

    ClassCollector cc;
    for (const auto &error : cc.collect(parser_, &ast)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // check inheritance loops
    for (const auto &error : check_loops(ast)) {
        errors.push_back(error);
    }

    // collect features
    parser_->reset();
    FeatureCollector mc;
    for (const auto &error : mc.collect_methods(parser_, &ast)) {
        errors.push_back(error);
    }

    if (mc.fatal()) {
        return unexpected(errors);
    }

    // check overwrite correctness
    for (const auto &error : check_overwrites(ast)) {
        errors.push_back(error);
    }

    // typecheck
    parser_->reset();
    for (const auto &error : TypeChecker().check(lexer_, parser_, &ast)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // return the typed AST
    return std::move(ast);
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

vector<std::string> check_loops(Classes &tree) {
    vector<std::string> errors;

    std::unordered_set<Type> cycle_classes;
    vector<vector<std::string>> cycles;
    for (Type t : tree.get_types()) {
        if (cycle_classes.count(t)) continue;
        Type curr = tree.get_parent(t);
        std::unordered_set<Type> visited;
        visited.insert(t);
        while (curr != tree.no_type) {
            if (visited.count(curr)) {
                Type end = curr;
                vector<std::string> cycle;
                do {
                    cycle.push_back(tree.get_name(curr));
                    cycle_classes.insert(curr);
                    curr = tree.get_parent(curr);
                }
                while (curr != end);
                cycles.push_back(cycle);
                break;
            }
            visited.insert(curr);
            curr = tree.get_parent(curr);
        }
    }

    if (!cycles.empty()) {
        errors.push_back(print_inheritance_loops_error(cycles));
    }
    return errors;
}

std::vector<std::string> check_overwrites(Classes &ast) {
    std::vector<std::string> errors;

    for (Type t : ast.get_types()) {
        Class *c = ast.get_class(t);

        // check attributes
        Attributes *attrs = c->get_attributes();
        for (auto it = attrs->begin(); it != attrs->end(); ++it) {
            Type earliest = ast.no_type;
            Type par = ast.get_parent(t);
            while (par != ast.no_type) {
                Attributes *parattrs = ast.get_class(par)->get_attributes();
                if (parattrs->contains(it->get_name())) {
                    earliest = par;
                }
                par = ast.get_parent(par);
            }
            if (earliest != ast.no_type) {
                std::stringstream ss;
                ss << "Attribute `" << it->get_name() << "` in class `" << c->get_name() <<
                    "` redefines attribute with the same name in ancestor `" <<
                    ast.get_class(earliest)->get_name() <<
                    "` (earliest ancestor that defines this attribute)";
                errors.push_back(ss.str());
            }
        }

        // check methods
        Methods *methods = c->get_methods();
        for (std::string methodname : methods->get_names()) {
            Type earliest = ast.no_type;
            Type par = ast.get_parent(t);
            while (par != ast.no_type) {
                Methods *parmethods = ast.get_class(par)->get_methods();
                if (parmethods->contains(methodname)) {
                    if (methods->get_signature(methodname) != parmethods->get_signature(methodname)) {
                        earliest = par;
                    }
                }
                par = ast.get_parent(par);
            }
            if (earliest != ast.no_type) {
                std::stringstream ss;
                ss << "Override for method " << methodname << " in class " <<
                    c->get_name() << " has different signature than method in ancestor " <<
                    ast.get_class(earliest)->get_name() << " (earliest ancestor that mismatches)";
                errors.push_back(ss.str());
            }
        }
    }

    return errors;
}
