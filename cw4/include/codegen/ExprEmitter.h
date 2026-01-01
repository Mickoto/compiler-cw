#ifndef CODEGEN_EXPR_EMITTER_H_
#define CODEGEN_EXPR_EMITTER_H_

#include <string>
#include <stack>
#include <unordered_map>

#include "semantics/typed-ast/Classes.h"
#include "semantics/typed-ast/Expr.h"
#include "ObjectModelTable.h"
#include "ConstantStorage.h"
#include "Location.h"

class ExprEmitter {
public:
    ExprEmitter(Classes *ast, ObjectModelTable *omt, ConstantStorage *cs) : ast(ast), omt(omt), cs(cs) {}

    void emit_expr(Expr *expr);

    size_t get_needed_stack_size(Expr *expr);

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

    std::stack<std::unordered_map<std::string, Location>> scopes;
    int fp_off_curr;

    void emit_expr_int(Expr *expr);
};

#endif
