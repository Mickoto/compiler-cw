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

void CoolCodegen::generate(ostream &out) {
    ObjectModelTable omt(ast.get());
    ConstantStorage cs;
    ExprEmitter ee(ast.get(), &omt, &cs);

    //text section
    riscv_emit::emit_text_segment_tag(out);
    ee.emit_all_methods(out);
    ee.emit_all_inits(out);

    // data section
    riscv_emit::emit_data_segment_tag(out);

    emit_object_prototypes(out, ast.get(), ee);
    emit_dispatch_tables(out, ast.get(), omt);
    emit_class_name_table(out, ast.get(), ee);

    // 8. emit static constants

    riscv_emit::emit_type_tag(out, "bool", ast->from_name("Bool"));
    riscv_emit::emit_type_tag(out, "int", ast->from_name("Int"));
    riscv_emit::emit_type_tag(out, "string", ast->from_name("String"));
    riscv_emit::emit_empty_line(out);

// .globl Main.main
// Main.main:
//     add fp, sp, 0
//     sw ra, 0(sp)
//     addi sp, sp, -4
//
//     sw fp, 0(sp)
//     addi sp, sp, -4
//     la t0, _string1.content
//     sw t0, 0(sp)
//     addi sp, sp, -4
//
//     jal IO.out_string
//
//     lw ra, 0(fp)
//     addi sp, sp, 8
//     lw fp, 0(sp)
//     ret
// 
// .data
// # ----------------- Init methods -----------------------------------------------
// .globl String_init
// String_init:
//
//     add fp, sp, 0
//     sw ra, 0(sp)
//     addi sp, sp, -4
//
//     sw s1, 0(sp)
//     addi sp, sp, -4
//     add s1, a0, zero
//
//     # copy Int prototype first
//
//     la a0, Int_protObj
//     sw fp, 0(sp)
//     addi sp, sp, -4
//
//     call Object.copy
//
//     sw a0, 12(s1)      # store new Int as length; value of Int is 0 by default
//
//     add a0, s1, zero   # restore String argument
//
//     addi sp, sp, 4
//     lw s1, 0(sp)
//     lw ra, 0(fp)
//     addi sp, sp, 8
//     lw fp, 0(sp)
//
//     ret
//
// # ------------- Class object table ---------------------------------------------
// class_objTab:
//     ...
//     .word -1 # GC tag
// _string1.length:
//     .word 2  # class tag;       2 for Int
//     .word 4  # object size;     4 words (16 bytes); GC tag not included
//     .word 0  # dispatch table;  Int has no methods
//     .word 13  # first attribute; value of Int
// 
//     .word -1 # GC tag
// _string1.content:
//     .word 4  # class tag;       4 for String
//     .word 8  # object size;     8 words (16 + 16 bytes); GC tag not included
//     .word String_dispTab
//     .word _string1.length # first attribute; pointer length
//     .string \"hello world!\" # includes terminating null char\n\
//     .byte 0
//     .byte 0

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

