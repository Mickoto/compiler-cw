#include "MethodChecker.h"
#include <sstream>

std::any MethodCollector::visitClass(CoolParser::ClassContext *ctx) {
    current = ast->from_name(ctx->classname->getText());
    visitChildren(ctx);

    return std::any {};
}

std::any MethodCollector::visitMethod(CoolParser::MethodContext *ctx) {
    Methods *methods = ast->get_class(current)->get_methods();
    std::string classname_ = ast->get_class(current)->get_name();
    std::string methodname = ctx->name->getText();
    std::vector<Type> signature;
    std::vector<std::string> argnames;
    for (auto f : ctx->formal()) {
        auto *type = f->define()->type();
        if (type->SELF_TYPE()) {
            signature.push_back(current);
            argnames.push_back(f->define()->OBJECTID()->getText());
        }
        else {
            std::string typename_ = type->TYPEID()->getText();
            if (!ast->contains(typename_)) {
                std::stringstream ss;
                ss << "Method `" << methodname << "` in class `" <<
                    classname_ << "` declared to have an argument of type `" <<
                    typename_ <<"` which is undefined";
                errors.push_back(ss.str());
                return std::any {};
            }
            signature.push_back(ast->from_name(typename_));
            argnames.push_back(f->define()->OBJECTID()->getText());
        }
    }

    auto *type = ctx->type();
    if (type->SELF_TYPE()) {
        signature.push_back(current);
    }
    else {
        std::string typename_ = type->TYPEID()->getText();
        if (!ast->contains(typename_)) {
            std::stringstream ss;
            ss << "Method `" << methodname << "` in class `" <<
                classname_ << "` declared to have return type `" <<
                typename_ <<"` which is undefined";
            errors.push_back(ss.str());
            return std::any {};
        }
        signature.push_back(ast->from_name(typename_));
    }

    if (methods->contains(methodname)) {
        std::stringstream ss;
        ss << "Method `" << methodname
            << "` already defined for class `" << classname_ << "`";
        errors.push_back(ss.str());
        return std::any {};
    }
    methods->add_method(Method(methodname, signature));
    methods->set_argument_names(methodname, argnames);

    return std::any {};
}

std::any MethodCollector::visitAttr(CoolParser::AttrContext *ctx) {
    Attributes *attrs = ast->get_class(current)->get_attributes();
    std::string classname_ = ast->get_class(current)->get_name();
    std::string attrname = ctx->define()->OBJECTID()->getText();
    Type attrtype;
    auto *type = ctx->define()->type();
    if (type->SELF_TYPE()) {
        attrtype = current;
    }
    else {
        std::string typename_ = type->TYPEID()->getText();
        if (!ast->contains(typename_)) {
            std::stringstream ss;
            ss << "Attribute `" << attrname << "` in class `" <<
                classname_ << "` declared to have type `" <<
                typename_ <<"` which is undefined";
            errors.push_back(ss.str());
            return std::any {};
        }
        attrtype = ast->from_name(typename_);

    }
    if (attrs->contains(attrname)) {
        std::stringstream ss;
        ss << "Attribute `" << attrname
            << "` already defined for class `" << classname_ << "`";
        errors.push_back(ss.str());
        return std::any {};
    }
    attrs->add(Attribute(attrname, attrtype));

    return std::any {};
}

std::vector<std::string> MethodCollector::collect_methods(CoolParser *parser, Classes *classes) {
    ast = classes;
    visitProgram(parser->program());

    return errors;
}

std::vector<std::string> checkOverwrites(Classes &ast) {
    std::vector<std::string> errors;

    for (Type t : ast.get_classes()) {
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
