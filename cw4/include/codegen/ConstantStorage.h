#ifndef CODEGEN_CONSTANT_STORAGE_H_
#define CODEGEN_CONSTANT_STORAGE_H_

#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
struct Constant {
    std::string label;
    T value;
};

class ConstantStorage {
public:
    std::string insert_bool_const(bool val);
    std::string insert_int_const(int val);
    std::string insert_string_const(std::string val);

    std::vector<Constant<bool>> get_bool_constants();
    std::vector<Constant<int>> get_int_constants();
    std::vector<Constant<std::pair<std::string, std::string>>> get_string_constants();

private:
    std::vector<int> int_constants;
    std::unordered_map<int, int> int_to_index;
    std::vector<std::pair<std::string, int>> string_constants;
    std::unordered_map<std::string, int> string_to_index;

    std::string int_const_label(int index);
    std::string string_const_label(int index);
};

#endif
