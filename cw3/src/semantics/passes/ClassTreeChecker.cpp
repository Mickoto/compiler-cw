#include "semantics/passes/ClassTreeChecker.h"

using std::vector;

std::expected<Classes, vector<std::string> > ClassesBuilder::build(CoolParser *parser) {
    ast = Classes();
    obj = ast.insert("Object", ast.no_type);
    ast.insert("Int", obj);
    ast.insert("Bool", obj);
    ast.insert("String", obj);
    ast.insert("IO", obj);
    visitProgram(parser->program());

    for (auto pair : supers) {
        if (!ast.contains(pair.second)) {
            errors.push_back(ast.get_class(pair.first)->get_name() + " inherits from undefined class " + pair.second);
        }
        else {
            ast.get_class(pair.first)->set_parent(ast.from_name(pair.second));
        }
    }

    if (errors.empty())
        return std::move(ast);
    else
        return std::unexpected(errors);
}

std::any ClassesBuilder::visitClass(CoolParser::ClassContext *ctx) {
    std::string name = ctx->classname->getText();
    if (ast.contains(name)) {
        errors.push_back(name + " redefined!");
    }

    Type t = ast.add(name);
    if (ctx->inherit)
        supers.push_back({t, ctx->inherit->getText()});
    else
        ast.get_class(t)->set_parent(obj);

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

vector<std::string> checkLoops(Classes &tree) {
    vector<std::string> errors;

    std::unordered_set<Type> cycle_classes;
    vector<vector<std::string>> cycles;
    for (Type t : tree.get_classes()) {
        if (cycle_classes.count(t)) continue;
        Type curr = tree.get_parent(t);
        std::unordered_set<Type> visited;
        visited.insert(t);
        while (curr != tree.no_type) {
            if (visited.count(curr)) {
                Type end = curr;
                vector<std::string> cycle;
                do {
                    cycle.push_back(tree.get_class(curr)->get_name());
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
