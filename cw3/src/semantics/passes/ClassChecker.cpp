#include "semantics/passes/ClassChecker.h"
#include <iterator>

bool TypeTree::contains(const std::string& classname) const {
    return name_to_idx.count(classname);
}

void TypeTree::insert(const std::string& classname, ClassInfo info) {
    name_to_idx[classname] = classes.size();
    classes.push_back(info);
}

std::vector<ClassInfo> TypeTree::get_classes() const { return classes; }

ClassInfo TypeTree::getParent(const std::string& name) const { 
    int idx = name_to_idx.at(name);
    int paridx = name_to_idx.at(classes[idx].super);
    return classes[paridx];
}

TypeTree TypeTreeBuilder::build(CoolParser *parser) {
    type_tree = TypeTree();
    type_tree.insert("Int", {"Object"});
    type_tree.insert("Bool", {"Object"});
    type_tree.insert("String", {"Object"});
    type_tree.insert("IO", {"Object"});
    visitProgram(parser->program());

    return std::move(type_tree);
}

std::any TypeTreeBuilder::visitClass(CoolParser::ClassContext *ctx) {
    if (type_tree.contains(ctx->classname->getText())) {
        errors.push_back("No");
    }
    type_tree.insert(ctx->classname->getText(), {ctx->inherit->getText()});

    return std::any {};
}

std::vector<std::string> checkUndefined(TypeTree &tree) {
    std::vector<std::string> errors;
    for (auto c : tree.get_classes()) {
        if (!tree.contains(c.name)) {
            // TODO: add an error
        }
    }
    return errors;
}

std::vector<std::string> checkCycles(TypeTree &tree) {
    std::vector<std::string> errors;
    for (auto c : tree.get_classes()) {
        auto curr = tree.getParent(c.name);
        std::unordered_set<std::string> visited;
        visited.insert(c.name);
        while (curr.name != "Object") {
            if (visited.count(curr.name)) {
                // TODO: cycle detected!
            }
            visited.insert(curr.name);
            curr = tree.getParent(curr.name);
        }
    }
    return errors;
}
std::vector<std::string> checkOverwrites(TypeTree &tree);
