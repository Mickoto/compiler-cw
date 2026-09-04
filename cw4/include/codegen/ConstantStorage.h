#ifndef CODEGEN_CONSTANT_STORAGE_H_
#define CODEGEN_CONSTANT_STORAGE_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "semantics/typed-ast/Expr.h"

template <typename T>
struct Constant {
    std::string label;
    T value;
};

class ConstantStorage {
public:
    std::string get_bool_const(bool val);
    std::string insert_int_const(int val);
    std::string insert_string_const(std::string val);
    std::string insert_cases(std::vector<Type> val);
    void set_filename(std::string filename);

    std::vector<Constant<bool>> get_bool_constants();
    std::vector<Constant<int>> get_int_constants();
    std::vector<Constant<std::pair<std::string, std::string>>> get_string_constants();
    std::vector<Constant<std::vector<Type>>> get_case_tables();
    std::string get_filename();

private:
    std::vector<int> int_constants;
    std::unordered_map<int, int> int_to_index;
    std::vector<std::pair<std::string, int>> string_constants;
    std::unordered_map<std::string, int> string_to_index;
    std::vector<std::vector<Type>> case_tables;
    int filename_index;

    std::string int_const_label(int index);
    std::string string_const_label(int index);
    std::string case_table_label(int index);
};

#endif
