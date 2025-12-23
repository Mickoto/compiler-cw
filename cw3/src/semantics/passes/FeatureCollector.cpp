#include "FeatureCollector.h"
#include <sstream>

void FeatureCollector::initialize_base_classes() {
    // TODO:
}

std::any FeatureCollector::visitClass(CoolParser::ClassContext *ctx) {
    current = ast->from_name(ctx->classname->getText());
    visitChildren(ctx);

    return std::any {};
}

std::any FeatureCollector::visitMethod(CoolParser::MethodContext *ctx) {
    Methods *methods = ast->get_class(current)->get_methods();
    std::string curr_name = ast->get_name(current);
    std::string methodname = ctx->name->getText();
    std::vector<Type> signature;
    std::vector<std::string> argnames;
    for (auto f : ctx->formal()) {
        std::string typename_ = f->define()->TYPEID()->getText();
        if (!ast->contains(typename_)) {
            std::stringstream ss;
            ss << "Method `" << methodname << "` in class `" <<
                curr_name << "` declared to have an argument of type `" <<
                typename_ <<"` which is undefined";
            errors.push_back(ss.str());
            fatal_ = true;
            return std::any {};
        }
        signature.push_back(ast->from_name(typename_));
        argnames.push_back(f->define()->OBJECTID()->getText());
    }

    std::string typename_ = ctx->TYPEID()->getText();
    Type type_;
    if (!ast->contains(typename_)) {
        std::stringstream ss;
        ss << "Method `" << methodname << "` in class `" <<
            curr_name << "` declared to have return type `" <<
            typename_ <<"` which is undefined";
        errors.push_back(ss.str());
        fatal_ = true;
        return std::any {};
    }
    else {
        type_ = ast->from_name(typename_);
    }
    signature.push_back(type_);

    if (methods->contains(methodname)) {
        std::stringstream ss;
        ss << "Method `" << methodname
            << "` already defined for class `" << curr_name << "`";
        errors.push_back(ss.str());
        return std::any {};
    }
    methods->add_method(Method(methodname, signature));
    methods->set_argument_names(methodname, argnames);

    return std::any {};
}

std::any FeatureCollector::visitAttr(CoolParser::AttrContext *ctx) {
    Attributes *attrs = ast->get_class(current)->get_attributes();
    std::string curr_name = ast->get_name(current);
    std::string attrname = ctx->define()->OBJECTID()->getText();
    Type attrtype;
    std::string typename_ = ctx->define()->TYPEID()->getText();
    if (!ast->contains(typename_)) {
        std::stringstream ss;
        ss << "Attribute `" << attrname << "` in class `" <<
            curr_name << "` declared to have type `" <<
            typename_ <<"` which is undefined";
        errors.push_back(ss.str());
        fatal_ = true;
        return std::any {};
    }
    else {
        attrtype = ast->from_name(typename_);
    }

    if (attrs->contains(attrname)) {
        std::stringstream ss;
        ss << "Attribute `" << attrname
            << "` already defined for class `" << curr_name << "`";
        errors.push_back(ss.str());
        return std::any {};
    }
    attrs->add(Attribute(attrname, attrtype));

    return std::any {};
}

std::vector<std::string> FeatureCollector::collect_methods(CoolParser *parser, Classes *classes) {
    ast = classes;
    visitProgram(parser->program());

    return errors;
}
