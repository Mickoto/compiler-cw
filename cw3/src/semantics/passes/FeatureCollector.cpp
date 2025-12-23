#include "FeatureCollector.h"
#include <sstream>

void FeatureCollector::initialize_base_classes() {
    Type obj_type = ast->from_name("Object");
    Type int_type = ast->from_name("Int");
    Type bool_type = ast->from_name("Bool");
    Type string_type = ast->from_name("String");
    Type io_type = ast->from_name("IO");

    // Initialize Object
    Methods *obj_methods = ast->get_class(obj_type)->get_methods();
    {
        std::vector<Type> signature({obj_type});
        std::vector<std::string> args;
        obj_methods->add_method(Method("abort", signature));
        obj_methods->set_argument_names("abort", args);
    }
    {
        std::vector<Type> signature({string_type});
        std::vector<std::string> args;
        obj_methods->add_method(Method("type_name", signature));
        obj_methods->set_argument_names("type_name", args);
    }
    {
        std::vector<Type> signature({ast->self_type});
        std::vector<std::string> args;
        obj_methods->add_method(Method("copy", signature));
        obj_methods->set_argument_names("copy", args);
    }

    // Initialize IO
    Methods *io_methods = ast->get_class(io_type)->get_methods();
    {
        std::vector<Type> signature({string_type, ast->self_type});
        std::vector<std::string> args({"x"});
        io_methods->add_method(Method("out_string", signature));
        io_methods->set_argument_names("out_string", args);
    }
    {
        std::vector<Type> signature({int_type, ast->self_type});
        std::vector<std::string> args({"x"});
        io_methods->add_method(Method("out_int", signature));
        io_methods->set_argument_names("out_int", args);
    }
    {
        std::vector<Type> signature({string_type});
        std::vector<std::string> args;
        io_methods->add_method(Method("in_string", signature));
        io_methods->set_argument_names("in_string", args);
    }
    {
        std::vector<Type> signature({int_type});
        std::vector<std::string> args;
        io_methods->add_method(Method("in_int", signature));
        io_methods->set_argument_names("in_int", args);
    }

    // Initialize String
    Methods *string_methods = ast->get_class(string_type)->get_methods();
    {
        std::vector<Type> signature({int_type});
        std::vector<std::string> args;
        string_methods->add_method(Method("length", signature));
        string_methods->set_argument_names("length", args);
    }
    {
        std::vector<Type> signature({string_type, string_type});
        std::vector<std::string> args({"s"});
        string_methods->add_method(Method("concat", signature));
        string_methods->set_argument_names("concat", args);
    }
    {
        std::vector<Type> signature({int_type, int_type, string_type});
        std::vector<std::string> args({"i", "l"});
        string_methods->add_method(Method("substr", signature));
        string_methods->set_argument_names("substr", args);
    }
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
        std::string argname = f->define()->OBJECTID()->getText();
        if (typename_ == "SELF_TYPE") {
            std::stringstream ss;
            ss << "Formal argument `" << argname <<
                "` declared of type `SELF_TYPE` which is not allowed";
            errors.push_back(ss.str());
            fatal_ = true;
            return std::any {};
        }
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
        argnames.push_back(argname);
    }

    std::string typename_ = ctx->TYPEID()->getText();
    Type type_;
    if (typename_ == "SELF_TYPE") {
        type_ = ast->self_type;
    }
    else if (!ast->contains(typename_)) {
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

    if (typename_ == "SELF_TYPE") {
        attrtype = ast->self_type;
    }
    else if (!ast->contains(typename_)) {
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
    initialize_base_classes();
    visitProgram(parser->program());

    return errors;
}
