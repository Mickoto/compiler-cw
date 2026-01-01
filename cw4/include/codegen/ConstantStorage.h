#ifndef CODEGEN_CONSTANT_STORAGE_H_
#define CODEGEN_CONSTANT_STORAGE_H_

#include <string>
#include <unordered_map>
#include <vector>

class ConstantStorage {
public:
    void add_int_constant(int val);
    void add_string_constant(std::string val);

    int find_int_constant(int val);
    int find_string_constant(std::string val);
    int find_bool_constant(std::string val);

private:
    std::vector<int> int_constants;
    std::unordered_map<int, int> int_to_index;
    std::vector<std::string> string_constants;
    std::unordered_map<std::string, int> string_to_index;
};

#endif
