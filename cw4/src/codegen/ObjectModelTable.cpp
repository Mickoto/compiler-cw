#include "ObjectModelTable.h"

ObjectModel collect_features(Classes *ast, Type t, Type curr, int &attr_off, int &method_off) {
    if (curr == ast->no_type) {
        return ObjectModel();
    }

    ObjectModel om = collect_features(ast, t, ast->get_parent(curr), attr_off, method_off);

    for (auto attr : ast->get_class(curr)->get_attributes()->get_names()) {
        om.all_attrs.push_back({curr, attr});
        om.attr_name_to_off.insert({attr, attr_off++});
    }
    for (auto method : ast->get_class(curr)->get_methods()->get_names()) {
        if (om.method_name_to_off.count(method)) {
            om.all_methods[om.method_name_to_off[method]].owner = curr;
        }
        else {
            om.all_methods.push_back({curr, method});
            om.method_name_to_off.insert({method, method_off++});
        }
    }

    return om;
}

ObjectModelTable::ObjectModelTable(Classes *ast) {
    for (Type t : ast->get_types()) {
        int attroff = ATTR_START;
        int methodoff = METHOD_START;
        models.push_back(collect_features(ast, t, t, attroff, methodoff));
    }
}

int ObjectModelTable::get_attr_offset(Type type, const std::string &attr) const {
    return models[type].attr_name_to_off.at(attr) * WORD_SIZE;
}

int ObjectModelTable::get_method_offset(Type type, const std::string &method) const {
    return models[type].method_name_to_off.at(method) * WORD_SIZE;
}

ObjectModel::Feature ObjectModelTable::find_method(Type type, std::string method_name) {
    return models[type].all_methods.at(models[type].method_name_to_off.at(method_name));
}

std::vector<ObjectModel::Feature> ObjectModelTable::get_all_attrs(Type t) const {
    std::vector<ObjectModel::Feature> ret;
    for (auto feat : models[t].all_attrs) {
        ret.push_back(feat);
    }
    return std::move(ret);
}
std::vector<ObjectModel::Feature> ObjectModelTable::get_all_methods(Type t) const {
    std::vector<ObjectModel::Feature> ret;
    for (auto feat : models[t].all_methods) {
        ret.push_back(feat);
    }
    return std::move(ret);
}

bool ObjectModelTable::has_method(Type t, std::string method) const {
    return models[t].method_name_to_off.count(method);
}

bool ObjectModelTable::has_attr(Type t, std::string attr) const {
    return models[t].attr_name_to_off.count(attr);
}
