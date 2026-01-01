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

    // 3. emit code for method bodies; possibly append to static constants

    // 6. emit initialization methods for classes

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

// .globl Main.main\n\
// Main.main:\n\
//     # stack discipline:\n\
//     # caller:\n\
//     # - self object is passed in a0\n\
//     # - control link is pushed first on the stack\n\
//     # - arguments are pushed in reverse order on the stack\n\
//     # callee:\n\
//     # - activation frame starts at the stack pointer\n\
//     add fp, sp, 0\n\
//     # - previous return address is first on the activation frame\n\
//     sw ra, 0(sp)\n\
//     addi sp, sp, -4\n\
//     # - size of activation frame is fixed per method, because it depends on\n\
//     #   number of arguments\n\
//     # - fp points to sp\n\
//     # - sp points to next free stack memory\n\
//     # before using saved registers (s1 -- s11), push them on the stack\n\
// \n\
//     # stack discipline:\n\
//     # caller:\n\
//     # - self object is passed in a0\n\
//     # self already is a0, so no-op\n\
//     # - control link is pushed first on the stack\n\
//     sw fp, 0(sp)\n\
//     addi sp, sp, -4\n\
//     # - arguments are pushed in reverse order on the stack\n\
//     la t0, _string1.content\n\
//     sw t0, 0(sp)\n\
//     addi sp, sp, -4\n\
// \n\
//     jal IO.out_string\n\
// \n\
//     # stack discipline:\n\
//     # callee:\n\
//     # - restore used saved registers (s1 -- s11) from the stack\n\
//     # - ra is restored from first word on activation frame\n\
//     lw ra, 0(fp)\n\
//     # - ra, arguments, and control link are popped from the stack\n\
//     addi sp, sp, 8\n\
//     # - fp is restored from control link\n\
//     lw fp, 0(sp)\n\
//     # - result is stored in a0\n\
//     # caller:\n\
//     # - read return value from a0\n\
//     ret\n\
// \n\
// .data\n\
// # ----------------- Init methods -----------------------------------------------\n\
// \n\
// .globl Object_init\n\
// Object_init:\n\
//     # Most of the `init` functions of the default types are no-ops, so the\n\
//     # implementation is the same.\n\
// \n\
//     # stack discipline:\n\
//     # callee:\n\
//     # - activation frame starts at the stack pointer\n\
//     add fp, sp, 0\n\
//     # - previous return address is first on the activation frame\n\
//     sw ra, 0(sp)\n\
//     addi sp, sp, -4\n\
//     # before using saved registers (s1 -- s11), push them on the stack\n\
// \n\
//     # no op\n\
// \n\
//     # stack discipline:\n\
//     # callee:\n\
//     # - restore used saved registers (s1 -- s11) from the stack\n\
//     # - ra is restored from first word on activation frame\n\
//     lw ra, 0(fp)\n\
//     # - ra, arguments, and control link are popped from the stack\n\
//     addi sp, sp, 8\n\
//     # - fp is restored from control link\n\
//     lw fp, 0(sp)\n\
//     # - result is stored in a0\n\
// \n\
//     ret\n\
// \n\
// \n\
// # Initializes an object of class String passed in a0. Allocates a new Int to\n\
// # store the length of the String and links the length pointer to it. Returns the\n\
// # initialized String in a0.\n\
// #\n\
// # Used in `new String`, but useless, in general, since it creates an empty\n\
// # string. String only has methods `length`, `concat`, and `substr`.\n\
// .globl String_init\n\
// String_init:\n\
//     # In addition to the default behavior, copies the Int prototype object and\n\
//     # uses that as the length, rather than the prototype object directly. No\n\
//     # practical reason for this, other than simulating the default init logic for\n\
//     # an object with attributes.\n\
// \n\
//     add fp, sp, 0\n\
//     sw ra, 0(sp)\n\
//     addi sp, sp, -4\n\
// \n\
//     # store String argument\n\
//     sw s1, 0(sp)\n\
//     addi sp, sp, -4\n\
//     add s1, a0, zero\n\
// \n\
//     # copy Int prototype first\n\
// \n\
//     la a0, Int_protObj\n\
//     sw fp, 0(sp)\n\
//     addi sp, sp, -4\n\
// \n\
//     call Object.copy\n\
// \n\
//     sw a0, 12(s1)      # store new Int as length; value of Int is 0 by default\n\
// \n\
//     add a0, s1, zero   # restore String argument\n\
// \n\
//     addi sp, sp, 4\n\
//     lw s1, 0(sp)\n\
//     lw ra, 0(fp)\n\
//     addi sp, sp, 8\n\
//     lw fp, 0(sp)\n\
// \n\
//     ret\n\
// \n\
// # ------------- Class object table ---------------------------------------------\n\
// class_objTab:\n\
//     .word Object_protObj\n\
//     .word Object_init\n\
//     .word String_protObj\n\
//     .word String_init\n\
        ...
// \n\
//     .word -1 # GC tag\n\
// _string1.length:\n\
//     .word 2  # class tag;       2 for Int\n\
//     .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
//     .word 0  # dispatch table;  Int has no methods\n\
//     .word 13  # first attribute; value of Int\n\
// \n\
//     .word -1 # GC tag\n\
// _string1.content:\n\
//     .word 4  # class tag;       4 for String\n\
//     .word 8  # object size;     8 words (16 + 16 bytes); GC tag not included\n\
//     .word String_dispTab\n\
//     .word _string1.length # first attribute; pointer length\n\
//     .string \"hello world!\\n\" # includes terminating null char\n\
//     .byte 0\n\
//     .byte 0\n\
// ";

}

void emit_dispatch_tables(ostream &out, Classes *ast, ObjectModelTable &omt) {
    std::unordered_set<Type> builtins {
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
            riscv_emit::emit_word(out, method.owner + "." + method.name);
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
            t == ast->from_name("Main") ||
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
