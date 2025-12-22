#include "ClassCollector.h"

using std::vector;

void ClassCollector::instantiate_base_classes(Classes& ast) {
    obj = ast.insert("Object", ast.no_type);
    ast.insert("Int", obj);
    ast.insert("Bool", obj);
    ast.insert("String", obj);
    ast.insert("IO", obj);
}

void ClassCollector::link_inheritance(Classes& ast) {
    std::unordered_set uninheritable(
        {
            ast.from_name("Bool"),
            ast.from_name("Int"),
            ast.from_name("String")
        }
    );

    for (auto pair : supers) {
        if (!ast.contains(pair.second)) {
            std::stringstream ss;
            ss << ast.get_name(pair.first) << " inherits from undefined class " <<
                pair.second;
            errors.push_back(ss.str());
        }
        else if (uninheritable.count(ast.from_name(pair.second))) {
            std::stringstream ss;
            ss << "`" << ast.get_class(pair.first)->get_name() <<
                "` inherits from `" << pair.second << "` which is an error";
            errors.push_back(ss.str());
        }
        else {
            ast.get_class(pair.first)->set_parent(ast.from_name(pair.second));
        }
    }
}

vector<std::string> ClassCollector::collect(CoolParser *parser, Classes *ast) {
    this->ast = ast;
    instantiate_base_classes(ast);
    visitProgram(parser->program());

    link_inheritance(ast);

    return std::move(ast);
}

std::any ClassCollector::visitClass(CoolParser::ClassContext *ctx) {
    std::string name = ctx->classname->getText();
    if (name == "SELF_TYPE") {
        std::stringstream ss;
        ss << "Cannot define class `" << name << "`";
        errors.push_back(ss.str());
        return std::any {};
    }
    if (ast.contains(name)) {
        std::stringstream ss;
        ss << "Type `" << name << "` already defined";
        errors.push_back(ss.str());
        return std::any {};
    }

    Type t = ast.add(name);
    if (ctx->inherit)
        supers.push_back({t, ctx->inherit->getText()});
    else
        ast.get_class(t)->set_parent(obj);

    return std::any {};
}

