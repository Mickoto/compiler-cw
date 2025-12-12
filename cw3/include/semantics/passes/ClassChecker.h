#ifndef SEMANTICS_PASSES_CLASS_CHECKER_H_
#define SEMANTICS_PASSES_CLASS_CHECKER_H_

#include <any>
#include <string>
#include <map>
#include <vector>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

struct ClassInfo {
    std::string name;
    std::string super;
};

class TypeTree {

private:
    std::unordered_map<std::string, int> name_to_idx;
    std::vector<ClassInfo> classes;

public:
    bool contains(const std::string& classname) const;
    void insert(const std::string& classname, ClassInfo info);
    std::vector<ClassInfo> get_classes() const;
    ClassInfo getParent(const std::string& name) const;
};

class TypeTreeBuilder : public CoolParserBaseVisitor {
private:
    std::vector<std::string> errors;
    std::string current_class;
    TypeTree type_tree;

public:
    TypeTreeBuilder() {}

    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;
    virtual std::any visitMethod(CoolParser::MethodContext *ctx) override;

    // Checks hierarchy tree and method overwriting
    TypeTree build(CoolParser *parser);
};

std::vector<std::string> checkUndefined(TypeTree &tree);
std::vector<std::string> checkCycles(TypeTree &tree);
std::vector<std::string> checkOverwrites(TypeTree &tree);

#endif //SEMANTICS_PASSES_CLASS_CHECKER_H_
