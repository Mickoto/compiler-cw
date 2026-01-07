#include "ExprEmitter.h"
#include "CodeEmitter.h"
#include <unordered_set>

void ExprEmitter::scope_attrs(Type t) {
    scopes.push_front({});
    for (auto attr : omt->get_all_attrs(t)) {
        scopes.front().insert({attr.name, 
            {
                ast->get_class(attr.owner)->get_attributes()->get(attr.name)->get_type(),
                MemoryLocation {omt->get_attr_offset(t, attr.name), ArgumentRegister {0}}
            }
        });
    }
    scopes.front().insert({"self", {t, ArgumentRegister {0}}});
}

void emit_arithmetic(std::ostream &out, const Arithmetic *expr) {

}

void emit_assignment(std::ostream &out, const Assignment *expr) {

}

void emit_bool_constant_expr(std::ostream &out, const BoolConstant *expr) {
    
}

void emit_boolean_negation(std::ostream &out, const BooleanNegation *expr) {

}

void emit_case_of_esac(std::ostream &out, const CaseOfEsac *expr) {

}

void emit_dynamic_dispatch(std::ostream &out, const DynamicDispatch *expr) {

}

void emit_equality_comparison(std::ostream &out, const EqualityComparison *expr) {

}

void emit_if_then_else_fi(std::ostream &out, const IfThenElseFi *expr) {

}

void emit_int_constant_expr(std::ostream &out, const IntConstant *expr) {

}

void emit_integer_negation(std::ostream &out, const IntegerNegation *expr) {

}

void emit_is_void(std::ostream &out, const IsVoid *expr) {

}

// void emit_vardecl(std::ostream &out, const Arithmetic *expr);
void emit_let_in(std::ostream &out, const LetIn *expr) {

}

void emit_method_invocation(std::ostream &out, const MethodInvocation *expr) {

}

void emit_new_object(std::ostream &out, const NewObject *expr) {

}

void emit_object_reference(std::ostream &out, const ObjectReference *expr) {

}

void emit_parenthesized_expr(std::ostream &out, const ParenthesizedExpr *expr) {

}

void emit_sequence(std::ostream &out, const Sequence *expr) {

}

void emit_static_dispatch(std::ostream &out, const StaticDispatch *expr) {

}

void emit_string_constant_expr(std::ostream &out, const StringConstant *expr) {

}

void emit_while_loop_pool(std::ostream &out, const WhileLoopPool *expr) {

}

void emit_scope(std::ostream &out) {
    riscv_emit::emit_move(out, FramePointer {}, StackPointer {});
    riscv_emit::emit_push_register(out, ReturnAddress {});
    riscv_emit::emit_empty_line(out);
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

        scope_attrs(t);

        Methods *methods = ast->get_class(t)->get_methods();
        for (std::string methodname : methods->get_names()) {
            std::string label = ast->get_name(t) + "." + methodname;
            if (label == "Main.main") {
                riscv_emit::emit_globl(out, label);
            }
            riscv_emit::emit_label(out, label);
            riscv_emit::emit_move(out, FramePointer {}, StackPointer {});
            riscv_emit::emit_push_register(out, ReturnAddress {});

            riscv_emit::emit_empty_line(out);
            scopes.push_front({});
            std::vector<std::string> argnames = methods->get_argument_names(methodname);
            std::vector<Type> signature = methods->get_signature(methodname).value();
            for (int i = 0; i < argnames.size(); i++) {
                scopes.front().insert({argnames[i], {signature[i], MemoryLocation {WORD_SIZE * i, FramePointer {}}}});
            }
            emit_expr(out, methods->get_body(methodname));
            scopes.pop_front();
            riscv_emit::emit_empty_line(out);

            riscv_emit::emit_load_word(out, ReturnAddress {}, MemoryLocation {0, FramePointer {}});
            riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, 8);
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

        bool noop = true;
        std::string label = ast->get_name(t) + "_init";
        riscv_emit::emit_globl(out, label);
        riscv_emit::emit_label(out, label);

        Attributes *attrs = ast->get_class(t)->get_attributes();
        if (!builtins.count(t)) {
            for (auto attr : attrs->get_names()) {
                if (!attrs->has_initializer(attr)) continue;
                if (noop) {
                    emit_scope(out);
                    riscv_emit::emit_push_register(out, SavedRegister {0});
                    riscv_emit::emit_move(out, SavedRegister {0}, ArgumentRegister {0});
                }
                emit_expr(out, attrs->get_initializer(attr));
                riscv_emit::emit_store_word(out, ArgumentRegister {0}, MemoryLocation {omt->get_attr_offset(t, attr), SavedRegister {0}});
                riscv_emit::emit_move(out, ArgumentRegister {0}, SavedRegister {0});
                riscv_emit::emit_empty_line(out);
            }
        }

        if (!noop) {
            riscv_emit::emit_pop_into_register(out, SavedRegister {0});
            riscv_emit::emit_load_word(out, ReturnAddress {}, MemoryLocation {0, FramePointer {}});
            riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, 8);
        }
        else {
            riscv_emit::emit_add_immediate(out, StackPointer {}, StackPointer {}, 4);
        }
        riscv_emit::emit_load_word(out, FramePointer {}, MemoryLocation {0, StackPointer {}});
        riscv_emit::emit_return(out);
        riscv_emit::emit_empty_line(out);

        scopes.pop_front();
    }
}

void ExprEmitter::emit_expr(std::ostream& out, const Expr *expr) {

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

void ExprEmitter::emit_string_constant(std::ostream& out, std::string label, std::string value, std::string length_label) {
    riscv_emit::emit_gc_tag(out);

    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("String"));
    riscv_emit::emit_word(out, 4 + (value.length() + 3) / 4);
    riscv_emit::emit_word(out, "String_dispTab");
    riscv_emit::emit_word(out, length_label);

    riscv_emit::emit_string(out, value);
    for (int i = 0; (value.length() + i + 1) % 4 != 0; i++) {
        riscv_emit::emit_byte(out, 0);
    }
}

void ExprEmitter::emit_int_proto(std::ostream& out) {
    riscv_emit::emit_globl(out, "Int_protObj");
    emit_int_constant(out, "Int_protObj", 0);
}

void ExprEmitter::emit_bool_proto(std::ostream& out) {
    riscv_emit::emit_globl(out, "Bool_protObj");
    emit_int_constant(out, "Bool_protObj", false);
}

void ExprEmitter::emit_string_proto(std::ostream& out) {
    riscv_emit::emit_globl(out, "String_protObj");
    emit_string_constant(out, "String_protObj", "", "Int_protObj");
}

void ExprEmitter::emit_type_proto(std::ostream& out, Type type, bool global) {
    riscv_emit::emit_gc_tag(out);

    std::string class_name = ast->get_name(type);
    auto features = omt->get_all_attrs(type);

    if (global) {
        riscv_emit::emit_globl(out, class_name + "_protObj");
    }
    riscv_emit::emit_label(out, class_name + "_protObj");
    riscv_emit::emit_word(out, ast->from_name("String"));
    riscv_emit::emit_word(out, ATTR_START + features.size());
    riscv_emit::emit_word(out, class_name + "_dispTab");

    for (auto feature : features) {
        riscv_emit::emit_word(out, 0);
    }
}

