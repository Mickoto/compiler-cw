#include "ConstantStorage.h"
#include <sstream>

std::string ConstantStorage::insert_bool_const(bool val) {
    return val ? "_bool_true" : "_bool_false";
}

std::string ConstantStorage::insert_int_const(int val) {
    if (!int_to_index.count(val)) {
        int_to_index.insert({val, int_constants.size()});
        int_constants.push_back(val);
    }
    return int_const_label(int_to_index.at(val));
}

std::string ConstantStorage::insert_string_const(std::string val) {
    if (!string_to_index.count(val)) {
        insert_int_const(val.length());
        string_to_index.insert({val, string_constants.size()});
        string_constants.push_back({val, int_to_index.at(val.length())});
    }
    return string_const_label(string_to_index.at(val));
}

std::vector<Constant<bool>> ConstantStorage::get_bool_constants() {
    return {{"_bool_false", false}, {"_bool_true", true}};
}

std::vector<Constant<int>> ConstantStorage::get_int_constants() {
    std::vector<Constant<int>> ret;
    for (int i = 0; i < int_constants.size(); i++) {
        ret.push_back({int_const_label(i), int_constants.at(i)});
    }
    return ret;
}
std::vector<Constant<std::pair<std::string, std::string>>> ConstantStorage::get_string_constants() {
    std::vector<Constant<std::pair<std::string, std::string>>> ret;
    for (int i = 0; i < string_constants.size(); i++) {
        auto p = string_constants.at(i);
        ret.push_back({string_const_label(i), {p.first, int_const_label(p.second)}});
    }
    return ret;
}

std::string ConstantStorage::int_const_label(int index) {
    std::stringstream ss;
    ss << "_int" << index;
    return ss.str();
}

std::string ConstantStorage::string_const_label(int index) {
    std::stringstream ss;
    ss << "_string" << index;
    return ss.str();
}
