#include "ExprEmitter.h"
#include "CodeEmitter.h"
#include <sstream>
#include <unordered_set>
#include <iomanip>
#include <vector>

Type ExprEmitter::safe_get_type(const Expr *expr) {
    Type t = expr->get_type();
    if (t == ast->self_type) {
        t = last_self_type;
    }
    return t;
}

std::string ExprEmitter::make_unique_label() {
    std::stringstream ss;
    ss << "_L" << label++;
    return ss.str();
}

void ExprEmitter::scope_attrs(Type t) {
    scopes.push_front({});
    for (auto attr : omt->get_all_attrs(t)) {
        scopes.front().insert({attr.name,
            MemoryLocation {omt->get_attr_offset(t, attr.name), SavedRegister {1}}
        });
    }
    scopes.front().insert({"self", SavedRegister {1}});
}

Location ExprEmitter::find_reference(const std::string &varname) {
    for (auto scope : scopes) {
        if (scope.count(varname)) {
            return scope.at(varname);
        }
    }
    return NoLocation {};
}

void ExprEmitter::emit_push(std::ostream &out, Register reg) {
    riscv_emit::emit_push_register(out, reg);
    fp_offset -= WORD_SIZE;
}

void ExprEmitter::emit_pop(std::ostream &out, Register reg) {
    riscv_emit::emit_pop_into_register(out, reg);
    fp_offset += WORD_SIZE;
}

void ExprEmitter::emit_arithmetic(std::ostream &out, const Arithmetic *expr) {
    emit_expr(out, expr->get_lhs());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    emit_push(out, TempRegister {0});

    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});

    emit_expr(out, expr->get_rhs());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    emit_push(out, TempRegister {0});

    emit_new_obj(out, ast->from_name("Int"));

    emit_pop(out, TempRegister {1});
    emit_pop(out, TempRegister {0});
    switch (expr->get_kind()) {
        case Arithmetic::Kind::Addition:
            riscv_emit::emit_add(out, TempRegister {0}, TempRegister {0}, TempRegister {1});
            break;
        case Arithmetic::Kind::Subtraction:
            riscv_emit::emit_subtract(out, TempRegister {0}, TempRegister {0}, TempRegister {1});
            break;
        case Arithmetic::Kind::Multiplication:
            riscv_emit::emit_multiply(out, TempRegister {0}, TempRegister {0}, TempRegister {1});
            break;
        case Arithmetic::Kind::Division:
            riscv_emit::emit_divide(out, TempRegister {0}, TempRegister {0}, TempRegister {1});
            break;
    }
    riscv_emit::emit_store_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
}

void ExprEmitter::emit_assignment(std::ostream &out, const Assignment *expr) {
    emit_expr(out, expr->get_value());
    riscv_emit::emit_move(out, TempRegister {0}, ArgumentRegister {0});
    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    riscv_emit::emit_move_data_between_locations(out, TempRegister {0}, find_reference(expr->get_assignee_name()));
    riscv_emit::emit_move(out, ArgumentRegister {0}, TempRegister {0});
}

void ExprEmitter::emit_bool_constant_expr(std::ostream &out, const BoolConstant *expr) {
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(expr->get_value()));
}

void ExprEmitter::emit_boolean_negation(std::ostream &out, const BooleanNegation *expr) {
    std::string false_label = make_unique_label();
    std::string true_label = make_unique_label();
    emit_expr(out, expr->get_argument());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    riscv_emit::emit_branch_equal_zero(out, TempRegister {0}, false_label);
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(false));
    riscv_emit::emit_jump(out, true_label);
    riscv_emit::emit_label(out, false_label);
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(true));
    riscv_emit::emit_label(out, true_label);
}

void ExprEmitter::emit_case_of_esac(std::ostream &out, const CaseOfEsac *expr) {
    std::string end_label = make_unique_label();
    std::string error_skip_label = make_unique_label();
    std::string void_error_label = make_unique_label();
    std::vector<Type> type_table;
    scopes.push_front({});

    for (int i = 0; i < expr->get_cases().size(); i++) {
        const CaseOfEsac::Case &case_ = expr->get_cases()[i];
        type_table.push_back(case_.get_type());
        scopes.front().insert({case_.get_name(), MemoryLocation {fp_offset, FramePointer {}}});
    }

    emit_expr(out, expr->get_multiplex());
    riscv_emit::emit_branch_not_equal_zero(out, ArgumentRegister {}, void_error_label);
    riscv_emit::emit_push_register(out, FramePointer {});
    riscv_emit::emit_load_address(out, TempRegister {0}, cs->get_filename());
    riscv_emit::emit_push_register(out, TempRegister {});
    riscv_emit::emit_load_address(out, TempRegister {0}, cs->insert_int_const(expr->get_line()));
    riscv_emit::emit_push_register(out, TempRegister {});
    riscv_emit::emit_jump_and_link(out, "_case_abort_on_void");
    riscv_emit::emit_label(out, void_error_label);

    emit_push(out, ArgumentRegister {0});

    riscv_emit::emit_load_word(out, TempRegister {1}, MemoryLocation {0, ArgumentRegister {0}});
    riscv_emit::emit_load_address(out, TempRegister {2}, cs->insert_cases(type_table));
    riscv_emit::emit_jump_and_link(out, "case_get_case");

    riscv_emit::emit_branch_less_than_or_equal(out, ZeroRegister {}, TempRegister {0}, error_skip_label);
    riscv_emit::emit_push_register(out, FramePointer {});
    riscv_emit::emit_load_address(out, TempRegister {0}, cs->get_filename());
    riscv_emit::emit_push_register(out, TempRegister {});
    riscv_emit::emit_load_address(out, TempRegister {0}, cs->insert_int_const(expr->get_line()));
    riscv_emit::emit_push_register(out, TempRegister {});
    riscv_emit::emit_load_address(out, TempRegister {0}, ast->get_name(safe_get_type(expr->get_multiplex())) + "_className");
    riscv_emit::emit_push_register(out, TempRegister {});
    riscv_emit::emit_jump_and_link(out, "_case_abort_no_match");
    riscv_emit::emit_label(out, error_skip_label);

    for (int i = 0; i < expr->get_cases().size() - 1; i++) {
        std::string skip_expr_label = make_unique_label();
        riscv_emit::emit_branch_not_equal_zero(out, TempRegister {0}, skip_expr_label);
        emit_expr(out, expr->get_cases()[i].get_expr());
        riscv_emit::emit_jump(out, end_label);
        riscv_emit::emit_label(out, skip_expr_label);
        riscv_emit::emit_add_immediate(out, TempRegister {0}, TempRegister {0}, -1);
    }
    emit_expr(out, expr->get_cases().back().get_expr());
    riscv_emit::emit_label(out, end_label);
    riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, WORD_SIZE);
    fp_offset += WORD_SIZE;
    scopes.pop_front();
}

void ExprEmitter::emit_dynamic_dispatch(std::ostream &out, const DynamicDispatch *expr) {
    int fp_offset_save = fp_offset;

    emit_push(out, FramePointer {});
    for (auto arg : expr->get_arguments()) {
        emit_expr(out, arg);
        emit_push(out, ArgumentRegister {0});
        riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    }

    emit_expr(out, expr->get_target());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {DISP_TABLE_OFF, ArgumentRegister {0}});
    Type dispatcher_type = safe_get_type(expr->get_target());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {omt->get_method_offset(dispatcher_type, expr->get_method_name()), TempRegister {0}});
    riscv_emit::emit_jump_and_link_register(out, TempRegister {0});

    fp_offset = fp_offset_save;
    if (expr->get_type() == ast->self_type) {
        last_self_type = dispatcher_type;
    }
}

void ExprEmitter::emit_equality_comparison(std::ostream &out, const EqualityComparison *expr) {
    emit_push(out, FramePointer {});

    emit_expr(out, expr->get_lhs());
    emit_push(out, ArgumentRegister {0});

    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    emit_expr(out, expr->get_rhs());

    riscv_emit::emit_jump_and_link(out, "_compare_equal");
    fp_offset += 2 * WORD_SIZE;
}

void ExprEmitter::emit_if_then_else_fi(std::ostream &out, const IfThenElseFi *expr) {
    std::string else_label = make_unique_label();
    std::string then_label = make_unique_label();

    emit_expr(out, expr->get_condition());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});

    riscv_emit::emit_branch_equal_zero(out, TempRegister {0}, else_label);
    emit_expr(out, expr->get_then_expr());
    riscv_emit::emit_jump(out, then_label);
    riscv_emit::emit_label(out, else_label);
    emit_expr(out, expr->get_else_expr());
    riscv_emit::emit_label(out, then_label);
}

void ExprEmitter::emit_int_constant_expr(std::ostream &out, const IntConstant *expr) {
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->insert_int_const(expr->get_value()));
}

void ExprEmitter::emit_integer_comparison(std::ostream &out, const IntegerComparison *expr) {
    std::string true_label = make_unique_label();
    std::string end_label = make_unique_label();

    emit_expr(out, expr->get_lhs());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    emit_push(out, TempRegister {0});

    emit_expr(out, expr->get_rhs());
    riscv_emit::emit_load_word(out, TempRegister {1}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    emit_pop(out, TempRegister {0});

    switch (expr->get_kind()) {
        case IntegerComparison::Kind::LessThan:
            riscv_emit::emit_branch_less_than(out, TempRegister {0}, TempRegister {1}, true_label);
            break;
        case IntegerComparison::Kind::LessThanEqual:
            riscv_emit::emit_branch_less_than_or_equal(out, TempRegister {0}, TempRegister {1}, true_label);
            break;
    }
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(false));
    riscv_emit::emit_jump(out, end_label);
    riscv_emit::emit_label(out, true_label);
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(true));
    riscv_emit::emit_label(out, end_label);
}

void ExprEmitter::emit_integer_negation(std::ostream &out, const IntegerNegation *expr) {
    emit_expr(out, expr->get_argument());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    riscv_emit::emit_negate(out, TempRegister {0}, TempRegister {0});
    emit_push(out, TempRegister {0});

    emit_new_obj(out, ast->from_name("Int"));

    emit_pop(out, TempRegister {0});
    riscv_emit::emit_store_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
}

void ExprEmitter::emit_is_void(std::ostream &out, const IsVoid *expr) {
    std::string false_label = make_unique_label();
    std::string true_label = make_unique_label();
    emit_expr(out, expr->get_subject());
    riscv_emit::emit_branch_equal_zero(out, ArgumentRegister {0}, false_label);
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(false));
    riscv_emit::emit_jump(out, true_label);
    riscv_emit::emit_label(out, false_label);
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->get_bool_const(true));
    riscv_emit::emit_label(out, true_label);
}

void ExprEmitter::emit_new_obj(std::ostream &out, Type t) {
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, ast->get_name(t) + "_protObj");
    riscv_emit::emit_push_register(out, FramePointer {});
    riscv_emit::emit_jump_and_link(out, "Object.copy");
    riscv_emit::emit_push_register(out, FramePointer {});
    riscv_emit::emit_jump_and_link(out, ast->get_name(t) + "_init");
}

void ExprEmitter::emit_new_obj_self_type(std::ostream &out) {
    riscv_emit::emit_push_register(out, FramePointer {});
    emit_push(out, SavedRegister {1});
    riscv_emit::emit_load_address(out, SavedRegister {1}, "class_objTab");
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {0, ArgumentRegister {0}});
    riscv_emit::emit_load_immediate(out, TempRegister {1}, 8);
    riscv_emit::emit_multiply(out, TempRegister {0}, TempRegister {0}, TempRegister {1});
    riscv_emit::emit_add(out, SavedRegister {1}, SavedRegister {1}, TempRegister {0});
    riscv_emit::emit_load_word(out, ArgumentRegister {0}, MemoryLocation {0, SavedRegister {1}});

    riscv_emit::emit_jump_and_link(out, "Object.copy");

    riscv_emit::emit_push_register(out, FramePointer {});
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {WORD_SIZE, SavedRegister {1}});
    riscv_emit::emit_jump_and_link_register(out, TempRegister {0});
    emit_pop(out, SavedRegister {1});
}

void ExprEmitter::emit_vardecl(std::ostream &out, const Vardecl *vardecl) {
    Type vartype = vardecl->get_type();
    emit_default_initialize(out, vartype);
    scopes.front().insert({vardecl->get_name(), MemoryLocation {fp_offset, FramePointer {}}});
    emit_push(out, ArgumentRegister {0});
    if (vardecl->has_initializer()) {
        riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
        emit_expr(out, vardecl->get_initializer());
        riscv_emit::emit_store_word(out, ArgumentRegister {0}, MemoryLocation {fp_offset + WORD_SIZE, FramePointer {}});
    }
    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
}

void ExprEmitter::emit_let_in(std::ostream &out, const LetIn *expr) {
    scopes.push_front({});
    for (auto vardecl : expr->get_vardecls()) {
        emit_vardecl(out, vardecl);
    }

    emit_expr(out, expr->get_body());

    scopes.pop_front();
    int num_vars = expr->get_vardecls().size();
    riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, WORD_SIZE * num_vars);
    fp_offset += WORD_SIZE * num_vars;
}

void ExprEmitter::emit_method_invocation(std::ostream &out, const MethodInvocation *expr) {
    int fp_offset_save = fp_offset;

    emit_push(out, FramePointer {});
    for (auto arg : expr->get_arguments()) {
        emit_expr(out, arg);
        emit_push(out, ArgumentRegister {0});
        riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    }

    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {DISP_TABLE_OFF, ArgumentRegister {0}});
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {omt->get_method_offset(curr_type, expr->get_method_name()), TempRegister {0}});
    riscv_emit::emit_jump_and_link_register(out, TempRegister {0});

    fp_offset = fp_offset_save;

    if (expr->get_type() == ast->self_type) {
        last_self_type = curr_type;
    }
}

void ExprEmitter::emit_new_object(std::ostream &out, const NewObject *expr) {
    if (expr->get_type() == ast->self_type) {
        emit_new_obj_self_type(out);
    }
    else {
        emit_new_obj(out, expr->get_type());
    }
}

void ExprEmitter::emit_object_reference(std::ostream &out, const ObjectReference *expr) {
    riscv_emit::emit_move_data_between_locations(out, find_reference(expr->get_name()), ArgumentRegister {0});
    if (expr->get_type() == ast->self_type) {
        last_self_type = curr_type;
    }
}

void ExprEmitter::emit_parenthesized_expr(std::ostream &out, const ParenthesizedExpr *expr) {
    emit_expr(out, expr->get_contents());
}

void ExprEmitter::emit_sequence(std::ostream &out, const Sequence *expr) {
    auto sequence = expr->get_sequence();
    emit_expr(out, sequence.front());
    for (int i = 1; i < sequence.size(); i++) {
        riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
        emit_expr(out, sequence.at(i));
    }
}

void ExprEmitter::emit_static_dispatch(std::ostream &out, const StaticDispatch *expr) {
    int fp_offset_save = fp_offset;

    emit_push(out, FramePointer {});
    for (auto arg : expr->get_arguments()) {
        emit_expr(out, arg);
        emit_push(out, ArgumentRegister {0});
        riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    }

    emit_expr(out, expr->get_target());

    auto method = omt->find_method(expr->get_static_dispatch_type(), expr->get_method_name());
    riscv_emit::emit_jump_and_link(out, ast->get_name(method.owner) + "." + method.name);

    fp_offset = fp_offset_save;

    if (expr->get_type() == ast->self_type) {
        last_self_type = safe_get_type(expr->get_target());
    }
}

void ExprEmitter::emit_string_constant_expr(std::ostream &out, const StringConstant *expr) {
    riscv_emit::emit_load_address(out, ArgumentRegister {0}, cs->insert_string_const(expr->get_value()));
}

void ExprEmitter::emit_while_loop_pool(std::ostream &out, const WhileLoopPool *expr) {
    std::string exit_label = make_unique_label();
    std::string loop_label = make_unique_label();

    riscv_emit::emit_label(out, loop_label);
    emit_expr(out, expr->get_condition());
    riscv_emit::emit_load_word(out, TempRegister {0}, MemoryLocation {3 * WORD_SIZE, ArgumentRegister {0}});
    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    riscv_emit::emit_branch_equal_zero(out, TempRegister {0}, exit_label);

    emit_expr(out, expr->get_body());
    riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
    riscv_emit::emit_jump(out, loop_label);

    riscv_emit::emit_label(out, exit_label);
    riscv_emit::emit_move(out, ArgumentRegister {0}, ZeroRegister {});
}

void ExprEmitter::emit_default_initialize(std::ostream& out, Type t) {
    std::unordered_set<Type> default_initializable = std::unordered_set<Type> {
        ast->from_name("Int"),
        ast->from_name("Bool"),
        ast->from_name("String"),
    };

    if (default_initializable.contains(t)) {
        emit_new_obj(out, t);
    }
    else {
        riscv_emit::emit_move(out, ArgumentRegister {0}, ZeroRegister {});
    }
}

void ExprEmitter::emit_all_methods(std::ostream& out) {
    std::unordered_set<Type> builtins = std::unordered_set<Type> {
        ast->from_name("Object"),
        ast->from_name("Int"),
        ast->from_name("Bool"),
        ast->from_name("String"),
        ast->from_name("IO")
    };

    for (Type t : ast->get_types()) {
        if (builtins.count(t)) continue;

        curr_type = t;
        scope_attrs(t);

        Methods *methods = ast->get_class(t)->get_methods();
        for (std::string methodname : methods->get_names()) {
            std::string label = ast->get_name(t) + "." + methodname;
            if (label == "Main.main") {
                riscv_emit::emit_globl(out, label);
            }
            riscv_emit::emit_label(out, label);
            riscv_emit::emit_move(out, FramePointer {}, StackPointer {});
            fp_offset = 0;
            emit_push(out, ReturnAddress {});
            emit_push(out, SavedRegister {1});
            riscv_emit::emit_move(out, SavedRegister {1}, ArgumentRegister {0});

            riscv_emit::emit_empty_line(out);
            scopes.push_front({});
            std::vector<std::string> argnames = methods->get_argument_names(methodname);
            int numargs = argnames.size();
            std::vector<Type> signature = methods->get_signature(methodname).value();
            for (int i = 0; i < argnames.size(); i++) {
                scopes.front().insert({argnames[i], MemoryLocation {(numargs - i) * WORD_SIZE, FramePointer {}}});
            }
            emit_expr(out, methods->get_body(methodname));
            scopes.pop_front();
            riscv_emit::emit_empty_line(out);

            emit_pop(out, SavedRegister {1});
            riscv_emit::emit_load_word(out, ReturnAddress {}, MemoryLocation {0, FramePointer {}});
            riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, (numargs + 2) * WORD_SIZE);
            riscv_emit::emit_load_word(out, FramePointer {}, MemoryLocation {0, StackPointer {}});
            riscv_emit::emit_return(out);
            riscv_emit::emit_empty_line(out);
        }

        scopes.pop_front();
    }
}

void ExprEmitter::emit_all_inits(std::ostream& out) {
    std::unordered_set<Type> builtins = std::unordered_set<Type> {
        ast->from_name("Object"),
        ast->from_name("Int"),
        ast->from_name("Bool"),
        ast->from_name("String"),
        ast->from_name("IO")
    };

    for (Type t : ast->get_types()) {
        scope_attrs(t);
        curr_type = t;

        std::string label = ast->get_name(t) + "_init";
        riscv_emit::emit_globl(out, label);
        riscv_emit::emit_label(out, label);

        riscv_emit::emit_move(out, FramePointer {}, StackPointer {});
        fp_offset = 0;
        emit_push(out, ReturnAddress {});
        emit_push(out, SavedRegister {1});
        riscv_emit::emit_move(out, SavedRegister {1}, ArgumentRegister {0});
        if (!builtins.count(curr_type) && !builtins.count(ast->get_parent(curr_type))) {
            riscv_emit::emit_push_register(out, FramePointer {});
            riscv_emit::emit_jump_and_link(out, ast->get_name(ast->get_parent(curr_type)) + "_init");
        }
        riscv_emit::emit_empty_line(out);

        Attributes *attrs = ast->get_class(t)->get_attributes();
        if (!builtins.count(t)) {
            for (auto attr : attrs->get_names()) {
                if (!attrs->has_initializer(attr)) continue;
                emit_expr(out, attrs->get_initializer(attr));
                riscv_emit::emit_store_word(out, ArgumentRegister {0}, MemoryLocation {omt->get_attr_offset(t, attr), SavedRegister {1}});
                riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {1});
                riscv_emit::emit_empty_line(out);
            }
        }

        emit_pop(out, SavedRegister {1});
        riscv_emit::emit_load_word(out, ReturnAddress {}, MemoryLocation {0, FramePointer {}});
        riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, 8);
        riscv_emit::emit_load_word(out, FramePointer {}, MemoryLocation {0, StackPointer {}});
        riscv_emit::emit_return(out);
        riscv_emit::emit_empty_line(out);

        scopes.pop_front();
    }
}

void ExprEmitter::emit_expr(std::ostream& out, const Expr *expr) {
    if (auto assignment = dynamic_cast<const Assignment *>(expr)) {
        return emit_assignment(out, assignment);
    }
    if (auto arithmetic = dynamic_cast<const Arithmetic *>(expr)) {
        return emit_arithmetic(out, arithmetic);
    }
    if (auto assignment = dynamic_cast<const Assignment *>(expr)) {
        return emit_assignment(out, assignment);
    }
    if (auto bool_constant_expr = dynamic_cast<const BoolConstant *>(expr)) {
        return emit_bool_constant_expr(out, bool_constant_expr);
    }
    if (auto boolean_negation = dynamic_cast<const BooleanNegation *>(expr)) {
        return emit_boolean_negation(out, boolean_negation);
    }
    if (auto case_of_esac = dynamic_cast<const CaseOfEsac *>(expr)) {
        return emit_case_of_esac(out, case_of_esac);
    }
    if (auto dynamic_dispatch = dynamic_cast<const DynamicDispatch *>(expr)) {
        return emit_dynamic_dispatch(out, dynamic_dispatch);
    }
    if (auto equality_comparison = dynamic_cast<const EqualityComparison *>(expr)) {
        return emit_equality_comparison(out, equality_comparison);
    }
    if (auto if_then_else_fi = dynamic_cast<const IfThenElseFi *>(expr)) {
        return emit_if_then_else_fi(out, if_then_else_fi);
    }
    if (auto int_constant_expr = dynamic_cast<const IntConstant *>(expr)) {
        return emit_int_constant_expr(out, int_constant_expr);
    }
    if (auto integer_comparison = dynamic_cast<const IntegerComparison *>(expr)) {
        return emit_integer_comparison(out, integer_comparison);
    }
    if (auto integer_negation = dynamic_cast<const IntegerNegation *>(expr)) {
        return emit_integer_negation(out, integer_negation);
    }
    if (auto is_void = dynamic_cast<const IsVoid *>(expr)) {
        return emit_is_void(out, is_void);
    }
    if (auto let_in = dynamic_cast<const LetIn *>(expr)) {
        return emit_let_in(out, let_in);
    }
    if (auto method_invocation = dynamic_cast<const MethodInvocation *>(expr)) {
        return emit_method_invocation(out, method_invocation);
    }
    if (auto new_object = dynamic_cast<const NewObject *>(expr)) {
        return emit_new_object(out, new_object);
    }
    if (auto object_reference = dynamic_cast<const ObjectReference *>(expr)) {
        return emit_object_reference(out, object_reference);
    }
    if (auto parenthesized_expr = dynamic_cast<const ParenthesizedExpr *>(expr)) {
        return emit_parenthesized_expr(out, parenthesized_expr);
    }
    if (auto sequence = dynamic_cast<const Sequence *>(expr)) {
        return emit_sequence(out, sequence);
    }
    if (auto static_dispatch = dynamic_cast<const StaticDispatch *>(expr)) {
        return emit_static_dispatch(out, static_dispatch);
    }
    if (auto string_constant_expr = dynamic_cast<const StringConstant *>(expr)) {
        return emit_string_constant_expr(out, string_constant_expr);
    }
    if (auto while_loop_pool = dynamic_cast<const WhileLoopPool *>(expr)) {
        return emit_while_loop_pool(out, while_loop_pool);
    }
}

void ExprEmitter::emit_int_constant(std::ostream& out, std::string label, int value) {
    riscv_emit::emit_gc_tag(out);

    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("Int"));
    riscv_emit::emit_word(out, 4);
    riscv_emit::emit_word(out, "Int_dispTab");
    riscv_emit::emit_word(out, value);
}

void ExprEmitter::emit_bool_constant(std::ostream& out, std::string label, bool value) {
    riscv_emit::emit_gc_tag(out);

    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("Bool"));
    riscv_emit::emit_word(out, 4);
    riscv_emit::emit_word(out, "Bool_dispTab");
    riscv_emit::emit_word(out, value);
}

std::string to_pretty(const std::string &str) {
    std::stringstream ss;

    for (char c : str) {
        switch (c) {
        case '\b':
            ss << "\\b";
            break;
        case '\t':
            ss << "\\t";
            break;
        case '\n':
            ss << "\\n";
            break;
        case '\f':
            ss << "\\f";
            break;
        case '"':
        case '\\':
            ss << std::string("\\") + c;
            break;
        default:
            if (c < 32) {
                ss << "<0x" <<
                    std::setfill('0') << std::setw(2) << std::hex << (int)c << ">";
            }
            else {
                ss << c;
            }
            break;
        }
    }

    return ss.str();
}

void ExprEmitter::emit_string_constant(std::ostream& out, std::string label, std::string value, std::string length_label) {
    riscv_emit::emit_gc_tag(out);

    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("String"));
    riscv_emit::emit_word(out, 5 + (value.length() + 3) / 4);
    riscv_emit::emit_word(out, "String_dispTab");
    riscv_emit::emit_word(out, length_label);

    riscv_emit::emit_string(out, to_pretty(value));
    for (int i = 0; (value.length() + i + 1) % 4; i++) {
        riscv_emit::emit_byte(out, 0);
    }
}

void ExprEmitter::emit_int_proto(std::ostream& out) {
    riscv_emit::emit_globl(out, "Int_protObj");
    emit_int_constant(out, "Int_protObj", 0);
}

void ExprEmitter::emit_bool_proto(std::ostream& out) {
    riscv_emit::emit_globl(out, "Bool_protObj");
    emit_bool_constant(out, "Bool_protObj", false);
}

void ExprEmitter::emit_string_proto(std::ostream& out) {
    riscv_emit::emit_globl(out, "String_protObj");
    emit_string_constant(out, "String_protObj", "", "Int_protObj");
}

void ExprEmitter::emit_type_proto(std::ostream& out, Type type, bool global) {
    std::unordered_set<Type> default_initializable = std::unordered_set<Type> {
        ast->from_name("Int"),
        ast->from_name("Bool"),
        ast->from_name("String"),
    };

    riscv_emit::emit_gc_tag(out);

    std::string class_name = ast->get_name(type);
    auto features = omt->get_all_attrs(type);

    if (global) {
        riscv_emit::emit_globl(out, class_name + "_protObj");
    }
    riscv_emit::emit_label(out, class_name + "_protObj");
    riscv_emit::emit_word(out, type);
    riscv_emit::emit_word(out, ATTR_START + features.size());
    riscv_emit::emit_word(out, class_name + "_dispTab");

    for (auto feature : features) {
        Type attr_type = ast->get_class(feature.owner)->get_attributes()->get_type(feature.name).value();
        if (default_initializable.contains(attr_type)) {
            riscv_emit::emit_word(out, ast->get_name(attr_type) + "_protObj");
        }
        else {
            riscv_emit::emit_word(out, 0);
        }
    }
}

