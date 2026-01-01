#include "ExprEmitter.h"
#include "CodeEmitter.h"

void ExprEmitter::emit_int_constant(std::ostream& out, std::string label, int value) {
    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("Int"));
    riscv_emit::emit_word(out, 4);
    riscv_emit::emit_word(out, "Int_dispTab");
    riscv_emit::emit_word(out, value);
    riscv_emit::emit_gc_tag(out);
}

void ExprEmitter::emit_bool_constant(std::ostream& out, std::string label, bool value) {
    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("Bool"));
    riscv_emit::emit_word(out, 4);
    riscv_emit::emit_word(out, "Bool_dispTab");
    riscv_emit::emit_word(out, value);
    riscv_emit::emit_gc_tag(out);
}

void ExprEmitter::emit_string_constant(std::ostream& out, std::string label, std::string value, std::string length_label) {
    riscv_emit::emit_label(out, label);
    riscv_emit::emit_word(out, ast->from_name("String"));
    riscv_emit::emit_word(out, 4 + (value.length() + 3) / 4);
    riscv_emit::emit_word(out, "String_dispTab");
    riscv_emit::emit_word(out, length_label);

    riscv_emit::emit_string(out, value);
    for (int i = 0; (value.length() + i) % 4 != 0; i++) {
        riscv_emit::emit_byte(out, 0);
    }

    riscv_emit::emit_gc_tag(out);
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

    riscv_emit::emit_gc_tag(out);
}

