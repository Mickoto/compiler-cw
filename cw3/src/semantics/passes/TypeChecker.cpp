#include "TypeChecker.h"

#include <any>
#include <cstdlib>
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

Type TypeChecker::find_id_type(std::string& identifier) {
    for (auto it = scopes.begin(); it != scopes.end(); ++it) {
        if (it->count(identifier)) {
            return it->at(identifier);
        }
    }
    return ast->no_type;
}

any TypeChecker::visitClass(CoolParser::ClassContext *ctx) {
    current = ast->from_name(ctx->classname->getText());
    visitChildren(ctx);

    return any {};
}

any TypeChecker::visitMethod(CoolParser::MethodContext *ctx) {
    Methods *methods = ast->get_class(current)->get_methods();
    Attributes *attrs = ast->get_class(current)->get_attributes();
    std::string methodname = ctx->name->getText();

    scopes.push_front({});
    for (auto it = attrs->begin(); it != attrs->end(); ++it) {
        scopes.front().insert({ it->get_name(), it->get_type() });
    }
    std::vector<std::string> argnames = methods->get_argument_names(methodname);
    std::vector<Type> signature = methods->get_signature(methodname).value();
    for (int i = 0; i < argnames.size(); i++) {
        scopes.front().insert({ argnames[i], signature[i] });
    }

    visitExpr(ctx->body);
    unique_ptr<Expr> body = move(scratchpad_.top());

    scopes.pop_front();

    if (!ast->is_super(body->get_type(), methods->get_signature(methodname).value().back())) {
        stringstream ss;
        ss << "In class `" << ast->get_class(current)->get_name() <<
            "` method `" << methodname << "`: `" <<
            ast->get_class(body->get_type())->get_name() << "` is not `" <<
            ast->get_class(methods->get_signature(methodname).value().back())->get_name() <<
            "`: type of method body is not a subtype of return type";
        errors.push_back(ss.str());
    }
    scratchpad_.pop();
    methods->set_body(methodname, move(body));

    return any {};
}

any TypeChecker::visitAttr(CoolParser::AttrContext *ctx) {
    if (ctx->expr()) {
        Attributes *attrs = ast->get_class(current)->get_attributes();
        scopes.push_front({});
        for (auto it = attrs->begin(); it != attrs->end(); ++it) {
            scopes.front().insert({ it->get_name(), it->get_type() });
        }

        visitExpr(ctx->expr());

        scopes.pop_front();

        unique_ptr<Expr> init = move(scratchpad_.top());
        scratchpad_.pop();

        string attrname = ctx->define()->OBJECTID()->getText();
        if (!ast->is_super(init->get_type(), attrs->get_type(attrname).value())) {
            stringstream ss;
            ss << "In class `" << ast->get_class(current)->get_name() <<
                "` attribute `" << attrname << "`: `" <<
                ast->get_class(init->get_type())->get_name() <<
                "` is not `" <<
                ast->get_class(attrs->get_type(attrname).value())->get_name() <<
                "`: type of initialization expression is not a subtype of declared type";
            errors.push_back(ss.str());
        }
        ast->get_class(current)->get_attributes()->set_initializer(
            ctx->define()->OBJECTID()->getText(),
            move(init)
        );
    }

    return any {};
}

any TypeChecker::visitExpr(CoolParser::ExprContext *ctx) {
    if (ctx->obj) {
        return visitMemDispatch(ctx);
    } else if (ctx->name) {
        return visitDispatch(ctx);
    } else if (ctx->unop) {
        return visitUnop(ctx);
    } else if (ctx->binop) {
        return visitBinop(ctx);
    } else if (ctx->ASSIGN()) {
        return visitAssign(ctx);
    }
    return visitChildren(ctx);
}

any TypeChecker::visitMemDispatch(CoolParser::ExprContext *ctx) {
    string methodname = ctx->name->getText();
    visitExpr(ctx->obj);
    unique_ptr<Expr> obj = move(scratchpad_.top());
    scratchpad_.pop();

    Type dispatch_type = obj->get_type();
    if (ctx->type()) {
        string vartypename = ctx->type()->TYPEID()->getText();
        if (!ast->contains(vartypename)) {
            stringstream ss;

            errors.push_back(ss.str());
            dispatch_type = ast->error_type;
        }
        else {
            dispatch_type = ast->from_name(vartypename);
        }
    }

    vector<Type> signature = ast->get_class(dispatch_type)->get_methods()->get_signature(methodname).value();

    Type ret_type = signature.back();
    vector<unique_ptr<Expr>> args;
    for (int i = 0; i < ctx->args.size(); i++) {
        visitExpr(ctx->args[i]);
        unique_ptr<Expr> arg = move(scratchpad_.top());
        scratchpad_.pop();

        if (!ast->is_super(arg->get_type(), signature[i])) {
            stringstream ss;
            // TODO:
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
        args.push_back(move(arg));
    }

    if (ctx->type()) {
        scratchpad_.push(make_unique<StaticDispatch>(
            move(obj),
            dispatch_type,
            methodname,
            move(args),
            ret_type
        ));
    }
    else {
        scratchpad_.push(make_unique<DynamicDispatch>(
            move(obj),
            methodname,
            move(args),
            ret_type
        ));
    }

    return any{};
}

any TypeChecker::visitDispatch(CoolParser::ExprContext *ctx) {
    string methodname = ctx->name->getText();
    vector<Type> signature = ast->get_class(current)->get_methods()->get_signature(methodname).value();
    Type ret_type = signature.back();
    vector<unique_ptr<Expr>> args;
    for (int i = 0; i < ctx->args.size(); i++) {
        visitExpr(ctx->args[i]);
        unique_ptr<Expr> arg = move(scratchpad_.top());
        scratchpad_.pop();

        if (!ast->is_super(arg->get_type(), signature[i])) {
            stringstream ss;
            // TODO:
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
        args.push_back(move(arg));
    }

    scratchpad_.push(make_unique<MethodInvocation>(
        methodname,
        move(args),
        ret_type
    ));
    return any{};
}

any TypeChecker::visitIf(CoolParser::IfContext *ctx) {
    Type bool_type = ast->from_name("Bool");

    visitExpr(ctx->expr(0));
    std::unique_ptr<Expr> cond = move(scratchpad_.top());
    scratchpad_.pop();
    visitExpr(ctx->expr(1));
    std::unique_ptr<Expr> ifexpr = move(scratchpad_.top());
    scratchpad_.pop();
    visitExpr(ctx->expr(2));
    std::unique_ptr<Expr> elseexpr = move(scratchpad_.top());
    scratchpad_.pop();

    Type ret_type = ast->lub(ifexpr->get_type(), elseexpr->get_type());
    if (!ast->is_super(cond->get_type(), bool_type)) {
        stringstream ss;
        ss << "Type `" << ast->get_class(cond->get_type())->get_name() << "` of if-then-else-fi condition is not `Bool`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }
    scratchpad_.push(make_unique<IfThenElseFi>(
        move(cond), move(ifexpr), move(elseexpr), ret_type
    ));

    return any{};
}

any TypeChecker::visitWhile(CoolParser::WhileContext *ctx) {
    Type bool_type = ast->from_name("Bool");
    Type obj_type = ast->from_name("Object");

    visitExpr(ctx->expr(0));
    std::unique_ptr<Expr> cond = move(scratchpad_.top());
    scratchpad_.pop();
    visitExpr(ctx->expr(1));
    std::unique_ptr<Expr> body = move(scratchpad_.top());
    scratchpad_.pop();

    Type ret_type = obj_type;
    if (!ast->is_super(cond->get_type(), bool_type)) {
        stringstream ss;
        ss << "Type `" << ast->get_class(cond->get_type())->get_name() << "` of while-loop-pool condition is not `Bool`";
        errors.push_back(ss.str());
        ret_type = ast->error_type;
    }

    scratchpad_.push(make_unique<WhileLoopPool>(
        move(cond), move(body), ret_type
    ));

    return any{};
}

any TypeChecker::visitBlock(CoolParser::BlockContext *ctx) {
    vector<unique_ptr<Expr>> exprs;
    Type last;
    scopes.push_front({});
    for (auto expr : ctx->expr()) {
        visitExpr(expr);
        exprs.push_back(move(scratchpad_.top()));
        scratchpad_.pop();
        last = exprs.back()->get_type();
    }
    scopes.pop_front();

    scratchpad_.push(make_unique<Sequence>(move(exprs), last));

    return any{};
}

any TypeChecker::visitLet(CoolParser::LetContext *ctx) {
    vector<unique_ptr<Vardecl>> vardecls;

    scopes.push_front({});

    for (auto letdef : ctx->letdef()) {
        string varname = letdef->define()->OBJECTID()->getText();
        string vartypename = letdef->define()->type()->TYPEID()->getText();
        Type vartype;
        if (!ast->contains(vartypename)) {
            stringstream ss;

            errors.push_back(ss.str());
            vartype = ast->error_type;
        }
        else {
            vartype = ast->from_name(vartypename);
        }

        if (letdef->expr()) {
            visitExpr(letdef->expr());
            unique_ptr<Expr> init = move(scratchpad_.top());
            scratchpad_.pop();

            if (!ast->is_super(init->get_type(), vartype)) {
                stringstream ss;
                ss << "Initializer for variable `" << varname <<
                    "` in let-in expression is of type `" << ast->get_class(init->get_type())->get_name() <<
                    "` which is not a subtype of the declared type `" << vartypename << "`";
                errors.push_back(ss.str());
            }
            vardecls.push_back(
                make_unique<Vardecl>(
                    varname, move(init), vartype
                )
            );
        }
        else {
            vardecls.push_back(make_unique<Vardecl>(
                varname, vartype
            ));
        }
        scopes.front().insert({ varname, vartype });
    }

    visitExpr(ctx->expr());
    unique_ptr<Expr> body = move(scratchpad_.top());
    scratchpad_.pop();

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

any TypeChecker::visitCase(CoolParser::CaseContext *ctx) {
    visitExpr(ctx->expr());
    unique_ptr<Expr> multiplex = move(scratchpad_.top());
    scratchpad_.pop();
    vector<CaseOfEsac::Case> cases;
    unordered_set<Type> casetypes;

    Type ret_type = ast->error_type;
    for (int i = 0; i < ctx->branch().size(); i++) {
        scopes.push_front({});

        string varname = ctx->branch(i)->define()->OBJECTID()->getText();
        string vartypename = ctx->branch(i)->define()->type()->TYPEID()->getText();
        Type vartype;
        if (!ast->contains(vartypename)) {
            stringstream ss;
            ss << "Option `" << varname << "` in case-of-esac declared to have unknown type `" << vartypename << "`";
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
            // NOTE: bogus
            scopes.front().insert({ varname, vartype });
        }

        visitExpr(ctx->branch(i)->expr());
        unique_ptr<Expr> caseexpr = move(scratchpad_.top());
        scratchpad_.pop();

        ret_type = ast->lub(ret_type, caseexpr->get_type());

        cases.push_back(CaseOfEsac::Case(varname, vartype, move(caseexpr)));

        scopes.pop_front();
    }

    // TODO: figure out what line is
    scratchpad_.push(make_unique<CaseOfEsac>(move(multiplex), move(cases), 0, ret_type));

    return any{};
}

any TypeChecker::visitUnop(CoolParser::ExprContext *ctx) {
    if (ctx->unop->getText()[0] == 'n' &&
        ctx->unop->getText()[1] == 'e') {
        string typename_ = ctx->type()->TYPEID()->getText();
        Type ret_type;
        if (!ast->contains(typename_)) {
            stringstream ss;
            ss << "Attempting to instantiate unknown class `" << typename_ << "`";
            errors.push_back(ss.str());
            ret_type = ast->error_type;
        }
        else {
            ret_type = ast->from_name(typename_);
        }
        scratchpad_.push(make_unique<NewObject>(ret_type));
        return any {};
    }

    visitChildren(ctx);
    std::unique_ptr<Expr> arg = move(scratchpad_.top());
    scratchpad_.pop();
    Type bool_type = ast->from_name("Bool");
    Type int_type = ast->from_name("Int");

    switch (ctx->unop->getText()[0]) {
        case 'i':
        case 'I':
            scratchpad_.push(make_unique<IsVoid>(move(arg), bool_type));
            break;
        case 'n':
        case 'N':
        {
            Type ret_type = bool_type;
            if (!ast->is_super(arg->get_type(), bool_type)) {
                stringstream ss;
                ss << "Argument of boolean negation is not of type `Bool`, but of type `"
                    << ast->get_class(arg->get_type())->get_name() << "`";
                errors.push_back(ss.str());
                ret_type = ast->error_type;
            }
            scratchpad_.push(make_unique<BooleanNegation>(move(arg), ret_type));
            break;
        }
        case '~':
            Type ret_type = int_type;
            if (!ast->is_super(arg->get_type(), int_type)) {
                stringstream ss;
                ss << "Argument of integer negation is not of type `Int`, but of type `"
                    << ast->get_class(arg->get_type())->get_name() << "`";
                errors.push_back(ss.str());
                ret_type = ast->error_type;
            }
            scratchpad_.push(make_unique<IntegerNegation>(move(arg), ret_type));
            break;
    }

    return any{};
}

any TypeChecker::visitBinop(CoolParser::ExprContext *ctx) {
    Type int_type = ast->from_name("Int");
    Type bool_type = ast->from_name("Bool");
    visitChildren(ctx);
    std::unique_ptr<Expr> rhs = move(scratchpad_.top());
    scratchpad_.pop();
    std::unique_ptr<Expr> lhs = move(scratchpad_.top());
    scratchpad_.pop();

    switch (ctx->binop->getText()[0]) {
        case '+':
        case '-':
        case '*':
        case '/':
        {
            Type ret_type = int_type;
            if (!ast->is_super(lhs->get_type(), int_type)) {
                stringstream ss;
                ss << "Left-hand-side of arithmetic expression is not of type `Int`, but of type `"
                    << ast->get_class(lhs->get_type())->get_name() << "`";
                errors.push_back(ss.str());
                ret_type = ast->error_type;
            }
            if (!ast->is_super(rhs->get_type(), int_type)) {
                stringstream ss;
                ss << "Right-hand-side of arithmetic expression is not of type `Int`, but of type `"
                    << ast->get_class(rhs->get_type())->get_name() << "`";
                errors.push_back(ss.str());
                ret_type = ast->error_type;
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
            break;
        }
        case '<':
        {
            Type ret_type = bool_type;
            if (!ast->is_super(lhs->get_type(), int_type)) {
                stringstream ss;
                ss << "Left-hand-side of integer comparison is not of type `Int`, but of type `"
                    << ast->get_class(lhs->get_type())->get_name() << "`";
                errors.push_back(ss.str());
                ret_type = ast->error_type;
            }
            if (!ast->is_super(rhs->get_type(), int_type)) {
                stringstream ss;
                ss << "Right-hand-side of integer comparison is not of type `Int`, but of type `"
                    << ast->get_class(rhs->get_type())->get_name() << "`";
                errors.push_back(ss.str());
                ret_type = ast->error_type;
            }

            IntegerComparison::Kind kind = ctx->binop->getText().length() > 1 ? IntegerComparison::Kind::LessThanEqual : IntegerComparison::Kind::LessThan;
            scratchpad_.push(make_unique<IntegerComparison>(move(lhs), move(rhs), kind, bool_type));
            break;
        }
        default:
        {
            Type ret_type = bool_type;
            if (lhs->get_type() == int_type || lhs->get_type() == bool_type ||
                    lhs->get_type() == ast->from_name("String")) {
                if (lhs->get_type() != rhs->get_type()) {
                    stringstream ss;
                    ss << "A `" << ast->get_class(lhs->get_type())->get_name() <<
                        "` can only be compared to another `" <<
                        ast->get_class(lhs->get_type())->get_name() << "` and not to a `" <<
                        ast->get_class(rhs->get_type())->get_name() << "`";
                    errors.push_back(ss.str());
                    ret_type = ast->error_type;
                }
            }

            scratchpad_.push(make_unique<EqualityComparison>(move(lhs), move(rhs), ret_type));
            break;
        }
    }
    return any {};
}

any TypeChecker::visitAssign(CoolParser::ExprContext *ctx) {
    string var_name = ctx->OBJECTID()->getText();

    visitExpr(ctx->expr(0));
    std::unique_ptr<Expr> init = move(scratchpad_.top());
    scratchpad_.pop();

    Type ret_type = init->get_type();
    Type var_type = find_id_type(var_name);
    if (var_type == ast->no_type) {
        stringstream ss;
        ss << "Assignee named `" << var_name << "` not in scope";
        errors.push_back(ss.str());
    }
    else if (!ast->is_super(init->get_type(), var_type)) {
        stringstream ss;
        ss << "In class `" << ast->get_class(current)->get_name() <<
            "` assignee `" << var_name << "`: `" <<
            ast->get_class(init->get_type())->get_name() <<
            "` is not `" <<
            ast->get_class(var_type)->get_name() <<
            "`: type of initialization expression is not a subtype of object type";
        errors.push_back(ss.str());
        // NOTE: bogus
        ret_type = var_type;
    }
    scratchpad_.push(make_unique<Assignment>(var_name, move(init), ret_type));

    return any{};
}

any TypeChecker::visitObject(CoolParser::ObjectContext *ctx) {
    if (ctx->SELF()) {
        scratchpad_.push(
            make_unique<ObjectReference>("self", current)
        );
        return std::any {};
    }
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

any TypeChecker::visitInteger(CoolParser::IntegerContext *ctx) {
    scratchpad_.push(
        make_unique<IntConstant>(
            std::atoi(ctx->INT_CONST()->getText().c_str()),
            ast->from_name("Int")
        )
    );

    return any {};
}

any TypeChecker::visitBool(CoolParser::BoolContext *ctx) {
    scratchpad_.push(
        make_unique<BoolConstant>(
            lexer_->get_bool_value(ctx->BOOL_CONST()->getSymbol()->getStartIndex()),
            ast->from_name("Bool")
        )
    );
    return any {};
}

any TypeChecker::visitString(CoolParser::StringContext *ctx) {
    scratchpad_.push(
        make_unique<StringConstant>(
            lexer_->get_string_pretty(ctx->STR_CONST()->getSymbol()->getStartIndex()),
            ast->from_name("String")
        )
    );
    return any {};
}

vector<string> TypeChecker::check(CoolLexer *lexer, CoolParser *parser, Classes *ast) {
    this->ast = ast;
    lexer_ = lexer;
    visitProgram(parser->program());
    parser->reset();

    return std::move(errors);
}
