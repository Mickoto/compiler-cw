#ifndef CODEGEN_EXPR_EMITTER_H_
#define CODEGEN_EXPR_EMITTER_H_

#include <ostream>
#include <string>
#include <stack>
#include <unordered_map>

#include "ObjectModelTable.h"
#include "ConstantStorage.h"
#include "Location.h"
#include "semantics/typed-ast/Classes.h"
#include "semantics/typed-ast/Expr.h"
#include "semantics/typed-ast/Arithmetic.h"
#include "semantics/typed-ast/Assignment.h"
#include "semantics/typed-ast/BoolConstant.h"
#include "semantics/typed-ast/BooleanNegation.h"
#include "semantics/typed-ast/CaseOfEsac.h"
#include "semantics/typed-ast/DynamicDispatch.h"
#include "semantics/typed-ast/EqualityComparison.h"
#include "semantics/typed-ast/IfThenElseFi.h"
#include "semantics/typed-ast/IntConstant.h"
#include "semantics/typed-ast/IntegerNegation.h"
#include "semantics/typed-ast/IsVoid.h"
#include "semantics/typed-ast/LetIn.h"
#include "semantics/typed-ast/MethodInvocation.h"
#include "semantics/typed-ast/NewObject.h"
#include "semantics/typed-ast/ObjectReference.h"
#include "semantics/typed-ast/ParenthesizedExpr.h"
#include "semantics/typed-ast/Sequence.h"
#include "semantics/typed-ast/StaticDispatch.h"
#include "semantics/typed-ast/StringConstant.h"
#include "semantics/typed-ast/WhileLoopPool.h"
#include "semantics/typed-ast/IntegerComparison.h"

class ExprEmitter {
public:
    ExprEmitter(Classes *ast, ObjectModelTable *omt, ConstantStorage *cs) : ast(ast), omt(omt), cs(cs), label(0) {}

    void emit_all_methods(std::ostream& out);
    void emit_all_inits(std::ostream& out);
    void emit_expr(std::ostream& out, const Expr *expr);

    void emit_int_constant(std::ostream& out, std::string label, int value);
    void emit_bool_constant(std::ostream& out, std::string label, bool value);
    void emit_string_constant(std::ostream& out, std::string label, std::string value, std::string length_label);
    void emit_int_proto(std::ostream& out);
    void emit_bool_proto(std::ostream& out);
    void emit_string_proto(std::ostream& out);
    void emit_type_proto(std::ostream& out, Type type, bool global = false);

private:
    Classes *ast;
    ObjectModelTable *omt;
    ConstantStorage *cs;

    std::deque<std::unordered_map<std::string, Location>> scopes;
    Type curr_type, last_self_type;
    int fp_offset;
    int label;

    Type safe_get_type(const Expr *expr);
    std::string make_unique_label();
    void scope_attrs(Type t);
    Location find_reference(const std::string &varname);
    // void emit_scope(std::ostream &out);
    void emit_push(std::ostream &out, Register reg);
    void emit_pop(std::ostream &out, Register reg);
    void emit_arithmetic(std::ostream &out, const Arithmetic *expr);
    void emit_assignment(std::ostream &out, const Assignment *expr);
    void emit_bool_constant_expr(std::ostream &out, const BoolConstant *expr);
    void emit_boolean_negation(std::ostream &out, const BooleanNegation *expr);
    void emit_case_of_esac(std::ostream &out, const CaseOfEsac *expr);
    void emit_dynamic_dispatch(std::ostream &out, const DynamicDispatch *expr);
    void emit_equality_comparison(std::ostream &out, const EqualityComparison *expr);
    void emit_if_then_else_fi(std::ostream &out, const IfThenElseFi *expr);
    void emit_int_constant_expr(std::ostream &out, const IntConstant *expr);
    void emit_integer_comparison(std::ostream &out, const IntegerComparison *expr);
    void emit_integer_negation(std::ostream &out, const IntegerNegation *expr);
    void emit_is_void(std::ostream &out, const IsVoid *expr);
    void emit_new_obj(std::ostream &out, Type t);
    void emit_new_obj_self_type(std::ostream &out);
    void emit_vardecl(std::ostream &out, const Vardecl *vardecl);
    void emit_let_in(std::ostream &out, const LetIn *expr);
    void emit_method_invocation(std::ostream &out, const MethodInvocation *expr);
    void emit_new_object(std::ostream &out, const NewObject *expr);
    void emit_object_reference(std::ostream &out, const ObjectReference *expr);
    void emit_parenthesized_expr(std::ostream &out, const ParenthesizedExpr *expr);
    void emit_sequence(std::ostream &out, const Sequence *expr);
    void emit_static_dispatch(std::ostream &out, const StaticDispatch *expr);
    void emit_string_constant_expr(std::ostream &out, const StringConstant *expr);
    void emit_while_loop_pool(std::ostream &out, const WhileLoopPool *expr);
};

#endif
