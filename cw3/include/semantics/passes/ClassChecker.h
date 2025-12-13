#ifndef SEMANTICS_PASSES_CLASS_CHECKER_H_
#define SEMANTICS_PASSES_CLASS_CHECKER_H_

#include <any>
#include <expected>
#include <string>
#include <map>
#include <vector>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

typedef int Type;

struct ClassInfo {
    std::string name;
    Type super;
};

class TypeTree {

private:
    std::unordered_map<std::string, int> name_to_idx;
    std::vector<ClassInfo> classes;

public:
    static const Type root_type = -1;

    bool contains(const std::string& classname) const;
    Type add_class(const std::string& classname);
    void set_super(Type base, Type super);
    Type insert(const std::string& classname, Type super);

    std::vector<Type> get_classes() const;
    Type from_name(const std::string& classname) const;
    ClassInfo& getInfo(Type t);
    Type getParent(Type t) const;
    bool isSuper(Type t, Type sup);
    Type lub(Type t1, Type t2);
};

class TypeTreeBuilder : public CoolParserBaseVisitor {
private:
    std::vector<std::string> errors;

    TypeTree type_tree;
    Type current_class;
    std::vector<std::pair<Type, std::string>> supers;

public:
    TypeTreeBuilder() {}

    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;
    // virtual std::any visitMethod(CoolParser::MethodContext *ctx) override;
    // virtual std::any visitAttr(CoolParser::AttrContext *ctx) override;

    std::expected<TypeTree, std::vector<std::string>> build(CoolParser *parser);
};

std::vector<std::string> checkLoops(TypeTree &tree);
std::vector<std::string> checkOverwrites(TypeTree &tree);

#endif //SEMANTICS_PASSES_CLASS_CHECKER_H_
