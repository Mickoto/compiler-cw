#include "CoolCodegen.h"
#include "CodeEmitter.h"
#include "ObjectModelTable.h"
#include "ConstantStorage.h"
#include "ExprEmitter.h"
#include <unordered_set>

using namespace std;

void emit_dispatch_tables(ostream &out, Classes *ast, ObjectModelTable &omt);
void emit_class_name_table(ostream &out, Classes *ast, ExprEmitter &ee);
void emit_object_prototypes(ostream &out, Classes *ast, ExprEmitter &ee);
void emit_constants(ostream &out, ExprEmitter &ee, ConstantStorage &cs);
void emit_helper_functions(ostream &out);
void emit_hierarchy_table(ostream &out, Classes *ast);

void CoolCodegen::generate(ostream &out) {
    ObjectModelTable omt(ast.get());
    ConstantStorage cs;
    cs.set_filename(file_name_);
    ExprEmitter ee(ast.get(), &omt, &cs);

    //text section
    riscv_emit::emit_text_segment_tag(out);
    ee.emit_all_methods(out);
    ee.emit_all_inits(out);
    emit_helper_functions(out);

    // data section
    riscv_emit::emit_data_segment_tag(out);
    emit_object_prototypes(out, ast.get(), ee);
    emit_dispatch_tables(out, ast.get(), omt);
    emit_class_name_table(out, ast.get(), ee);
    emit_hierarchy_table(out, ast.get());
    emit_constants(out, ee, cs);

    riscv_emit::emit_type_tag(out, "bool", ast->from_name("Bool"));
    riscv_emit::emit_type_tag(out, "int", ast->from_name("Int"));
    riscv_emit::emit_type_tag(out, "string", ast->from_name("String"));
    riscv_emit::emit_empty_line(out);
}

void emit_dispatch_tables(ostream &out, Classes *ast, ObjectModelTable &omt) {
    std::unordered_set<Type> builtins = std::unordered_set<Type> {
        ast->from_name("Object"),
        ast->from_name("Int"),
        ast->from_name("Bool"),
        ast->from_name("String"),
        ast->from_name("IO")
    };


    riscv_emit::emit_header_comment(out, "Dispatch tables");
    for (Type t : ast->get_types()) {
        std::string label = ast->get_name(t) + "_dispTab";
        if (builtins.count(t)) {
            riscv_emit::emit_globl(out, label);
        }
        riscv_emit::emit_label(out, label);
        for (auto method : omt.get_all_methods(t)) {
            riscv_emit::emit_word(out, ast->get_name(method.owner) + "." + method.name);
        }
        riscv_emit::emit_empty_line(out);
    }
}

void emit_class_name_table(ostream &out, Classes *ast, ExprEmitter &ee) {
    riscv_emit::emit_header_comment(out, "Name table of classes");
    riscv_emit::emit_p2align(out, 2);

    riscv_emit::emit_globl(out, "class_nameTab");
    riscv_emit::emit_label(out, "class_nameTab");
    for (Type t : ast->get_types()) {
        riscv_emit::emit_word(out, ast->get_name(t) + "_className");
    }

    for (Type t : ast->get_types()) {
        std::string name = ast->get_name(t);
        ee.emit_int_constant(out, name + "_classNameLength", name.length());
        ee.emit_string_constant(out, name + "_className", name, name + "_classNameLength");
        riscv_emit::emit_empty_line(out);
    }
}

void emit_object_prototypes(ostream &out, Classes *ast, ExprEmitter &ee) {
    riscv_emit::emit_header_comment(out, "Prototype objects");
    riscv_emit::emit_p2align(out, 2);

    for (Type t : ast->get_types()) {
        if (t == ast->from_name("Int")) {
            ee.emit_int_proto(out);
        }
        else if (t == ast->from_name("Bool")) {
            ee.emit_bool_proto(out);
        }
        else if (t == ast->from_name("String")) {
            ee.emit_string_proto(out);
        }
        else if (
            ast->contains("Main") && t == ast->from_name("Main") ||
            t == ast->from_name("Object") ||
            t == ast->from_name("IO")
        ) {
            ee.emit_type_proto(out, t, true);
        }
        else {
            ee.emit_type_proto(out, t);
        }
        riscv_emit::emit_empty_line(out);
    }
}

void emit_constants(ostream &out, ExprEmitter &ee, ConstantStorage &cs) {
    for (auto type_table : cs.get_case_tables()) {
        riscv_emit::emit_label(out, type_table.label);
        for (Type t : type_table.value) {
            riscv_emit::emit_word(out, t);
        }
        riscv_emit::emit_word(out, -1);
    }
    riscv_emit::emit_empty_line(out);
    for (auto bool_const : cs.get_bool_constants()) {
        ee.emit_bool_constant(out, bool_const.label, bool_const.value);
    }
    riscv_emit::emit_empty_line(out);
    for (auto int_const : cs.get_int_constants()) {
        ee.emit_int_constant(out, int_const.label, int_const.value);
    }
    riscv_emit::emit_empty_line(out);
    for (auto string_const : cs.get_string_constants()) {
        ee.emit_string_constant(out, string_const.label, string_const.value.first, string_const.value.second);
    }
    riscv_emit::emit_empty_line(out);
}

void emit_helper_functions(ostream &out) {
    out <<
        "case_get_case:\n"
            "\tli t4, -1\n"
            "\tloop1:\n"
            "\tbeq t1, t4, error\n"
            "\tadd t0, zero, zero\n"
            "\tloop2:\n"
            "\tli t3, 4\n"
            "\tmul t3, t0, t3\n"
            "\tadd t3, t3, t2\n"
            "\tlw t3, 0(t3)\n"
            "\tbeq t3, t4, endloop2\n"
            "\tbeq t3, t1, done\n"
            "\taddi t0, t0, 1\n"
            "\tj loop2\n"
            "\tendloop2:\n"
            "\tli t3, 4\n"
            "\tmul t3, t3, t1\n"
            "\tla t5, hierarchy_tab\n"
            "\tadd t3, t3, t5\n"
            "\tlw t1, 0(t3)\n"
            "\tj loop1\n"
            "\terror:\n"
            "\tadd t0, t4, zero\n"
            "\tdone:\n"
            "\tret\n";
}

void emit_hierarchy_table(ostream &out, Classes *ast) {
    riscv_emit::emit_header_comment(out, "Hierarchy table");
    riscv_emit::emit_label(out, "hierarchy_tab");
    for (Type t : ast->get_types()) {
        riscv_emit::emit_word(out, ast->get_parent(t));
    }
    riscv_emit::emit_empty_line(out);
}
