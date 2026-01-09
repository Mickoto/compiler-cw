#ifndef CODEGEN_MNEMONIC_H_
#define CODEGEN_MNEMONIC_H_

enum class Mnemonic {
    Add,
    AddImmediate,
    Subtract,
    Multiply,
    Divide,
    XorImmediate,
    ShiftLeftLogicalImmediate,
    SetEqualZero,
    SetLessThan,
    StoreWord,
    LoadImm,
    LoadWord,
    LoadAddress,
    Jump,
    JumpAndLink,
    Call,
    JumpAndLinkRegister,
    Return,
    BranchEqualZero,
    BranchNotEqualZero,
    BranchLessThanZero,
    BranchGreaterThanZero,
    BranchLessThan,
    BranchLessThanOrEqual
};

#endif
