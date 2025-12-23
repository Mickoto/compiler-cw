#include "ExprCollector.h"

#include <any>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "CoolParser.h"
#include "StringConstant.h"
#include "BoolConstant.h"
#include "IntConstant.h"
#include "ObjectReference.h"
#include "Arithmetic.h"
#include "IntegerComparison.h"
#include "EqualityComparison.h"
#include "IsVoid.h"
#include "IntegerNegation.h"
#include "BooleanNegation.h"
#include "NewObject.h"
#include "IfThenElseFi.h"
#include "WhileLoopPool.h"
#include "Sequence.h"
#include "Assignment.h"
#include "MethodInvocation.h"
#include "CaseOfEsac.h"
#include "Vardecl.h"
#include "LetIn.h"
#include "DynamicDispatch.h"
#include "StaticDispatch.h"

using namespace std;

Type ExprCollector::find_id_type(std::string& identifier) {
    for (auto it = scopes.begin(); it != scopes.end(); ++it) {
        if (it->count(identifier)) {
            return it->at(identifier);
        }
    }
    return ast->no_type;
}

Methods* ExprCollector::find_nearest_methods_containing(Type t, std::string& method) {
    if (t == ast->self_type)
        t = current;
    while (t != ast->no_type) {
        Methods *curr = ast->get_class(t)->get_methods();
        if (curr->contains(method)) {
            return curr;
        }
        t = ast->get_parent(t);
    }
    return nullptr;
}

unique_ptr<Expr> ExprCollector::buildExpr(CoolParser::ExprContext *ctx) {
    visitExpr(ctx);
    unique_ptr<Expr> ret = move(scratchpad_.top());
    scratchpad_.pop();
    return move(ret);
}

any ExprCollector::visitClass(CoolParser::ClassContext *ctx) {
    current = ast->from_name(ctx->classname->getText());
    visitedMethods.clear();
    visitedAttrs.clear();
    visitChildren(ctx);

    return any {};
}

void ExprCollector::scope_attributes() {
    scopes.front().insert({ "self", (const Type)ast->self_type });
    Type t = current;
    while (t != ast->no_type) {
        Attributes *attrs = ast->get_class(t)->get_attributes();
        for (auto it = attrs->begin(); it != attrs->end(); ++it) {
            scopes.front().insert({ it->get_name(), it->get_type() });
        }
        t = ast->get_parent(t);
    }
}

any ExprCollector::visitMethod(CoolParser::MethodContext *ctx) {
    string methodname = ctx->name->getText();

    if (visitedMethods.count(methodname)) {
        return any {};
    }
    visitedMethods.insert(methodname);

    scopes.push_front({});

    scope_attributes();

    Methods *methods = ast->get_class(current)->get_methods();
    std::vector<std::string> argnames = methods->get_argument_names(methodname);
    std::vector<Type> signature = methods->get_signature(methodname).value();
    for (int i = 0; i < argnames.size(); i++) {
        scopes.front().insert({ argnames[i], signature[i] });
    }

    unique_ptr<Expr> body = buildExpr(ctx->body);

    scopes.pop_front();

    if (!ast->is_super(current, body->get_type(), signature.back())) {
        stringstream ss;
        ss << "In class `" << ast->get_name(current) <<
            "` method `" << methodname << "`: `" <<
            ast->get_name(body->get_type()) << "` is not `" <<
            ast->get_name(signature.back()) << "`: type of method body is not a subtype of return type";
        errors.push_back(ss.str());
    }
    methods->set_body(methodname, move(body));

    return any {};
}

any ExprCollector::visitAttr(CoolParser::AttrContext *ctx) {
    string attrname = ctx->define()->OBJECTID()->getText();

    if (visitedAttrs.count(attrname)) {
        return any {};
    }
    visitedAttrs.insert(attrname);

    if (ctx->expr()) {
        Attributes *attrs = ast->get_class(current)->get_attributes();

        scopes.push_front({});
        scope_attributes();

        unique_ptr<Expr> init = buildExpr(ctx->expr());

        scopes.pop_front();

        if (!ast->is_super(current, init->get_type(), attrs->get_type(attrname).value())) {
            stringstream ss;
            ss << "In class `" << ast->get_name(current) <<
                "` attribute `" << attrname << "`: `" <<
                ast->get_name(init->get_type()) <<
                "` is not `" <<
                ast->get_name(attrs->get_type(attrname).value()) <<
                "`: type of initialization expression is not a subtype of declared type";
            errors.push_back(ss.str());
        }

        attrs->set_initializer(attrname, move(init));
    }

    return any {};
}

any ExprCollector::visitExpr(CoolParser::ExprContext *ctx) {
    if (ctx->obj) {
        visitMemDispatch(ctx);
    } else if (ctx->name) {
        visitDispatch(ctx);
    } else if (ctx->unop) {
        visitUnop(ctx);
    } else if (ctx->binop) {
        visitBinop(ctx);
    } else if (ctx->ASSIGN()) {
        visitAssign(ctx);
    }
    else {
        visitChildren(ctx);
    }

    return any {};
}

void ExprCollector::visitMemDispatch(CoolParser::ExprContext *ctx) {
    string methodname = ctx->name->getText();
    vector<unique_ptr<Expr>> args;
    Type ret_type = ast->error_type;
    unique_ptr<Expr> obj = buildExpr(ctx->obj);
    Type dispatch_type;
    Methods *methods;
    vector<Type> signature;
    bool invalid_call = false;

    if (obj->get_type() == ast->error_type) {
        goto abort;
    }

    dispatch_type = obj->get_type();
    if (ctx->TYPEID()) {
        string vartypename = ctx->TYPEID()->getText();
        if (!ast->contains(vartypename)) {
            stringstream ss;
            ss << "Undefined type `" << vartypename << "` in static method dispatch";
            errors.push_back(ss.str());
            // NOTE: bogus
            // goto abort;
        }
        else {
            dispatch_type = ast->from_name(vartypename);
        }

        if (!ast->is_super(current, obj->get_type(), dispatch_type)) {
            stringstream ss;
            ss << "`" << ast->get_name(obj->get_type()) <<
                "` is not a subtype of `" << vartypename << "`";
            errors.push_back(ss.str());
        }
    }

    methods = find_nearest_methods_containing(dispatch_type, methodname);
    if (!methods) {
        stringstream ss;
        ss << "Method `" << methodname << "` not defined for type `" << ast->get_name(dispatch_type) <<
            "` in " << (ctx->TYPEID() ? "static" : "dynamic") << " dispatch";
        errors.push_back(ss.str());
        goto abort;
    }
    signature = methods->get_signature(methodname).value();

    ret_type = signature.back();
    if (ret_type == ast->self_type) {
        ret_type = obj->get_type();
    }

    if (ctx->args.size() + 1 != signature.size()) {
        stringstream ss;
        ss << "Method `" << methodname << "` of type `" <<
            ast->get_name(dispatch_type) << "` called with the wrong number of arguments; " <<
            signature.size() - 1 << " arguments expected, but " <<
            ctx->args.size() << " provided";
        errors.push_back(ss.str());
        goto abort;
    }
    for (int i = 0; i < ctx->args.size(); i++) {
        unique_ptr<Expr> arg = buildExpr(ctx->args[i]);

        if (!ast->is_super(current, arg->get_type(), signature[i])) {
            stringstream ss;
            if (!invalid_call) {
                stringstream ss2;
                ss2 << "Invalid call to method `" << methodname << "` from class `" <<
                    ast->get_name(dispatch_type) << "`:";
                errors.push_back(ss2.str());
                invalid_call = true;
            }
            ss << "  `" << ast->get_name(arg->get_type()) << "` is not a subtype of `" <<
                ast->get_name(signature[i]) << "`: argument at position " <<
                i << " (0-indexed) has the wrong type";
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
        args.push_back(move(arg));
    }

    if (ctx->TYPEID()) {
        scratchpad_.push(make_unique<StaticDispatch>(
            move(obj),
            dispatch_type,
            methodname,
            move(args),
            ret_type
        ));
        return;
    }

abort:
    scratchpad_.push(make_unique<DynamicDispatch>(
        move(obj),
        methodname,
        move(args),
        ret_type
    ));
    return;
}

void ExprCollector::visitDispatch(CoolParser::ExprContext *ctx) {
    string methodname = ctx->name->getText();
    vector<unique_ptr<Expr>> args;
    Type ret_type = ast->error_type;
    vector<Type> signature;

    Methods *methods = find_nearest_methods_containing(current, methodname);
    if (!methods) {
        stringstream ss;
        // TODO:
        errors.push_back(ss.str());
        goto abort;
    }

    signature = methods->get_signature(methodname).value();
    if (ctx->args.size() + 1 != signature.size()) {
        stringstream ss;
        ss << "Method `" << methodname << "` of type `" <<
            ast->get_name(current) << "` called with the wrong number of arguments; " <<
            signature.size() - 1 << " arguments expected, but " <<
            ctx->args.size() << " provided";
        errors.push_back(ss.str());
        goto abort;
    }

    ret_type = signature.back();
    for (int i = 0; i < ctx->args.size(); i++) {
        visitExpr(ctx->args[i]);
        unique_ptr<Expr> arg = move(scratchpad_.top());
        scratchpad_.pop();

        if (!ast->is_super(current, arg->get_type(), signature[i])) {
            stringstream ss;
            // TODO:
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
        args.push_back(move(arg));
    }

abort:
    scratchpad_.push(make_unique<MethodInvocation>(
        methodname,
        move(args),
        ret_type
    ));
    return;
}

any ExprCollector::visitIf(CoolParser::IfContext *ctx) {
    Type bool_type = ast->from_name("Bool");

    std::unique_ptr<Expr> cond = buildExpr(ctx->expr(0));
    std::unique_ptr<Expr> ifexpr = buildExpr(ctx->expr(1));
    std::unique_ptr<Expr> elseexpr = buildExpr(ctx->expr(2));

    Type ret_type = ast->lub(current, ifexpr->get_type(), elseexpr->get_type());
    if (cond->get_type() != bool_type) {
        stringstream ss;
        ss << "Type `" << ast->get_name(cond->get_type()) << "` of if-then-else-fi condition is not `Bool`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }
    scratchpad_.push(make_unique<IfThenElseFi>(
        move(cond), move(ifexpr), move(elseexpr), ret_type
    ));

    return any{};
}

any ExprCollector::visitWhile(CoolParser::WhileContext *ctx) {
    Type bool_type = ast->from_name("Bool");
    Type obj_type = ast->from_name("Object");

    std::unique_ptr<Expr> cond = buildExpr(ctx->expr(0));
    std::unique_ptr<Expr> body = buildExpr(ctx->expr(1));

    Type ret_type = obj_type;
    if (cond->get_type() != bool_type) {
        stringstream ss;
        ss << "Type `" << ast->get_name(cond->get_type()) << "` of while-loop-pool condition is not `Bool`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }

    scratchpad_.push(make_unique<WhileLoopPool>(
        move(cond), move(body), ret_type
    ));

    return any{};
}

any ExprCollector::visitBlock(CoolParser::BlockContext *ctx) {
    vector<unique_ptr<Expr>> exprs;
    scopes.push_front({});
    for (auto expr : ctx->expr()) {
        exprs.push_back(buildExpr(expr));
    }
    scopes.pop_front();

    Type last = exprs.back()->get_type();
    scratchpad_.push(make_unique<Sequence>(move(exprs), last));

    return any{};
}

any ExprCollector::visitLet(CoolParser::LetContext *ctx) {
    vector<unique_ptr<Vardecl>> vardecls;

    scopes.push_front({});

    for (auto letdef : ctx->letdef()) {
        string varname = letdef->define()->OBJECTID()->getText();
        string vartypename = letdef->define()->TYPEID()->getText();
        Type vartype;
        if (vartypename == "SELF_TYPE") {
            vartype = ast->self_type;
        }
        else if (!ast->contains(vartypename)) {
            stringstream ss;
            // TODO:
            errors.push_back(ss.str());
            vartype = ast->error_type;
        }
        else {
            vartype = ast->from_name(vartypename);
        }

        if (letdef->expr()) {
            unique_ptr<Expr> init = buildExpr(letdef->expr());

            if (!ast->is_super(current, init->get_type(), vartype)) {
                stringstream ss;
                ss << "Initializer for variable `" << varname <<
                    "` in let-in expression is of type `" << ast->get_name(init->get_type()) <<
                    "` which is not a subtype of the declared type `" << vartypename << "`";
                errors.push_back(ss.str());
            }
            vardecls.push_back(
                make_unique<Vardecl>(varname, move(init), vartype)
            );
        }
        else {
            vardecls.push_back(
                make_unique<Vardecl>(varname, vartype)
            );
        }
        scopes.front().insert({ varname, vartype });
    }

    unique_ptr<Expr> body = buildExpr(ctx->expr());
    scopes.pop_front();

    scratchpad_.push(
        make_unique<LetIn>(
            move(vardecls),
            move(body),
            body->get_type()
        )
    );

    return any{};
}

any ExprCollector::visitCase(CoolParser::CaseContext *ctx) {
    unique_ptr<Expr> multiplex = buildExpr(ctx->expr());
    vector<CaseOfEsac::Case> cases;
    unordered_set<Type> casetypes;

    Type ret_type = ast->error_type;
    for (int i = 0; i < ctx->branch().size(); i++) {
        string varname = ctx->branch(i)->define()->OBJECTID()->getText();
        string vartypename = ctx->branch(i)->define()->TYPEID()->getText();
        Type vartype;

        scopes.push_front({});
        if (vartypename == "SELF_TYPE") {
            stringstream ss;
            ss << "`" << varname << "` in case-of-esac declared to be of type `SELF_TYPE` which is not allowed";
            errors.push_back(ss.str());
            vartype = ast->error_type;

        }
        else if (!ast->contains(vartypename)) {
            stringstream ss;
            ss << "Option `" << varname <<
                "` in case-of-esac declared to have unknown type `" << vartypename << "`";
            errors.push_back(ss.str());
            vartype = ast->error_type;
        }
        else {
            vartype = ast->from_name(vartypename);
            if (casetypes.count(vartype)) {
                stringstream ss;
                ss << "Multiple options match on type `" << vartypename << "`";
                errors.push_back(ss.str());
            }
            casetypes.insert(vartype);
            // NOTE: bogus placement
            scopes.front().insert({ varname, vartype });
        }

        unique_ptr<Expr> caseexpr = buildExpr(ctx->branch(i)->expr());
        scopes.pop_front();

        ret_type = ast->lub(current, ret_type, caseexpr->get_type());

        cases.push_back(CaseOfEsac::Case(varname, vartype, move(caseexpr)));
    }

    // TODO: figure out what line is
    scratchpad_.push(make_unique<CaseOfEsac>(move(multiplex), move(cases), 0, ret_type));

    return any{};
}

void ExprCollector::visitNew(CoolParser::ExprContext *ctx) {
        string typename_ = ctx->TYPEID()->getText();
        Type ret_type;
        if (typename_ == "SELF_TYPE") {
            ret_type = ast->self_type;
        }
        else if (!ast->contains(typename_)) {
            stringstream ss;
            ss << "Attempting to instantiate unknown class `" << typename_ << "`";
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
        else {
            ret_type = ast->from_name(typename_);
        }
        scratchpad_.push(make_unique<NewObject>(ret_type));
}

void ExprCollector::visitIsVoid(CoolParser::ExprContext *ctx) {
    std::unique_ptr<Expr> arg = buildExpr(ctx->expr(0));
    Type bool_type = ast->from_name("Bool");

    scratchpad_.push(make_unique<IsVoid>(move(arg), bool_type));
}

void ExprCollector::visitNeg(CoolParser::ExprContext *ctx) {
    std::unique_ptr<Expr> arg = buildExpr(ctx->expr(0));
    Type bool_type = ast->from_name("Bool");
    Type int_type = ast->from_name("Int");

    Type ret_type = int_type;
    if (!ast->is_super(current, arg->get_type(), int_type)) {
        stringstream ss;
        ss << "Argument of integer negation is not of type `Int`, but of type `"
            << ast->get_name(arg->get_type()) << "`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }
    scratchpad_.push(make_unique<IntegerNegation>(move(arg), ret_type));
}

void ExprCollector::visitNot(CoolParser::ExprContext *ctx) {
    std::unique_ptr<Expr> arg = buildExpr(ctx->expr(0));
    Type bool_type = ast->from_name("Bool");

    Type ret_type = bool_type;
    if (arg->get_type() != bool_type) {
        stringstream ss;
        ss << "Argument of boolean negation is not of type `Bool`, but of type `"
            << ast->get_name(arg->get_type()) << "`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }
    scratchpad_.push(make_unique<BooleanNegation>(move(arg), ret_type));
}

void ExprCollector::visitUnop(CoolParser::ExprContext *ctx) {
    if (ctx->unop->getText()[0] == 'n' &&
        ctx->unop->getText()[1] == 'e') {
        visitNew(ctx);
        return;
    }

    switch (ctx->unop->getText()[0]) {
        case 'i':
        case 'I':
            visitIsVoid(ctx);
            break;
        case 'n':
        case 'N':
            visitNot(ctx);
            break;
        case '~':
            visitNeg(ctx);
            break;
    }
}

void ExprCollector::visitArithmetic(CoolParser::ExprContext *ctx) {
    Type int_type = ast->from_name("Int");
    std::unique_ptr<Expr> lhs = buildExpr(ctx->expr(0));
    std::unique_ptr<Expr> rhs = buildExpr(ctx->expr(1));

    Type ret_type = int_type;
    if (lhs->get_type() != int_type) {
        stringstream ss;
        ss << "Left-hand-side of arithmetic expression is not of type `Int`, but of type `"
            << ast->get_name(lhs->get_type()) << "`";
        errors.push_back(ss.str());
        // ret_type = ast->error_type;
    }
    if (rhs->get_type() != int_type) {
        stringstream ss;
        ss << "Right-hand-side of arithmetic expression is not of type `Int`, but of type `"
            << ast->get_name(rhs->get_type()) << "`";
        errors.push_back(ss.str());
        // ret_type = ast->error_type;
    }

    Arithmetic::Kind kind;
    switch (ctx->binop->getText()[0]) {
        case '+':
            kind = Arithmetic::Kind::Addition;
            break;
        case '-':
            kind = Arithmetic::Kind::Subtraction;
            break;
        case '*':
            kind = Arithmetic::Kind::Multiplication;
            break;
        case '/':
            kind = Arithmetic::Kind::Division;
            break;
    }
    scratchpad_.push(make_unique<Arithmetic>(move(lhs), move(rhs), kind, ret_type));
}

void ExprCollector::visitComparison(CoolParser::ExprContext *ctx) {
    Type int_type = ast->from_name("Int");
    Type bool_type = ast->from_name("Bool");
    std::unique_ptr<Expr> lhs = buildExpr(ctx->expr(0));
    std::unique_ptr<Expr> rhs = buildExpr(ctx->expr(1));

    Type ret_type = bool_type;
    if (lhs->get_type() != int_type) {
        stringstream ss;
        ss << "Left-hand-side of integer comparison is not of type `Int`, but of type `"
            << ast->get_name(lhs->get_type()) << "`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }
    if (rhs->get_type() != int_type) {
        stringstream ss;
        ss << "Right-hand-side of integer comparison is not of type `Int`, but of type `"
            << ast->get_name(rhs->get_type()) << "`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }

    IntegerComparison::Kind kind = ctx->binop->getText().length() > 1 ? IntegerComparison::Kind::LessThanEqual : IntegerComparison::Kind::LessThan;
    scratchpad_.push(make_unique<IntegerComparison>(move(lhs), move(rhs), kind, bool_type));
}

void ExprCollector::visitEq(CoolParser::ExprContext *ctx) {
    Type int_type = ast->from_name("Int");
    Type bool_type = ast->from_name("Bool");
    Type string_type = ast->from_name("String");
    std::unique_ptr<Expr> lhs = buildExpr(ctx->expr(0));
    std::unique_ptr<Expr> rhs = buildExpr(ctx->expr(1));

    Type ret_type = bool_type;
    if (lhs->get_type() == int_type || lhs->get_type() == bool_type ||
                                       lhs->get_type() == string_type) {
        if (lhs->get_type() != rhs->get_type()) {
            stringstream ss;
            ss << "A `" << ast->get_name(lhs->get_type()) <<
                "` can only be compared to another `" <<
                ast->get_name(lhs->get_type()) << "` and not to a `" <<
                ast->get_name(rhs->get_type()) << "`";
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
    }

    scratchpad_.push(make_unique<EqualityComparison>(move(lhs), move(rhs), ret_type));
}

void ExprCollector::visitBinop(CoolParser::ExprContext *ctx) {
    switch (ctx->binop->getText()[0]) {
        case '+':
        case '-':
        case '*':
        case '/':
            visitArithmetic(ctx);
            break;
        case '<':
            visitComparison(ctx);
            break;
        default:
            visitEq(ctx);
            break;
    }
}

void ExprCollector::visitAssign(CoolParser::ExprContext *ctx) {
    string var_name = ctx->OBJECTID()->getText();

    std::unique_ptr<Expr> init = buildExpr(ctx->expr(0));

    Type ret_type = init->get_type();
    Type var_type = find_id_type(var_name);
    if (var_type == ast->no_type) {
        stringstream ss;
        ss << "Assignee named `" << var_name << "` not in scope";
        errors.push_back(ss.str());
    }
    else if (!ast->is_super(current, init->get_type(), var_type)) {
        stringstream ss;
        ss << "In class `" << ast->get_name(current) <<
            "` assignee `" << var_name << "`: `" <<
            ast->get_name(init->get_type()) <<
            "` is not `" <<
            ast->get_name(var_type) <<
            "`: type of initialization expression is not a subtype of object type";
        errors.push_back(ss.str());
        // NOTE: bogus
        ret_type = var_type;
    }
    scratchpad_.push(make_unique<Assignment>(var_name, move(init), ret_type));
}

any ExprCollector::visitObject(CoolParser::ObjectContext *ctx) {
    std::string obj_name = ctx->OBJECTID()->getText();
    Type obj_type = find_id_type(obj_name);
    if (obj_type == ast->no_type) {
        stringstream ss;
        ss << "Variable named `" << obj_name << "` not in scope";
        errors.push_back(ss.str());
        obj_type = ast->error_type;
    }
    scratchpad_.push(
        make_unique<ObjectReference>(obj_name, obj_type)
    );
    return any {};

}

any ExprCollector::visitInteger(CoolParser::IntegerContext *ctx) {
    scratchpad_.push(
        make_unique<IntConstant>(
            std::atoi(ctx->INT_CONST()->getText().c_str()),
            ast->from_name("Int")
        )
    );

    return any {};
}

any ExprCollector::visitBool(CoolParser::BoolContext *ctx) {
    scratchpad_.push(
        make_unique<BoolConstant>(
            lexer_->get_bool_value(ctx->BOOL_CONST()->getSymbol()->getStartIndex()),
            ast->from_name("Bool")
        )
    );
    return any {};
}

any ExprCollector::visitString(CoolParser::StringContext *ctx) {
    scratchpad_.push(
        make_unique<StringConstant>(
            lexer_->get_string_pretty(ctx->STR_CONST()->getSymbol()->getStartIndex()),
            ast->from_name("String")
        )
    );
    return any {};
}

vector<string> ExprCollector::check(CoolLexer *lexer, CoolParser *parser, Classes *ast) {
    this->ast = ast;
    lexer_ = lexer;
    visitProgram(parser->program());
    parser->reset();

    return std::move(errors);
}

