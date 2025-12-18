#include "CoolSemantics.h"

#include <expected>
#include <string>
#include <vector>

#include "passes/ClassTreeChecker.h"
#include "passes/MethodChecker.h"
#include "passes/TypeChecker.h"

using namespace std;

// Runs semantic analysis and returns a list of errors, if any.
//
// TODO: change the type from void * to your typed AST type
expected<Classes, vector<string>> CoolSemantics::run() {
    vector<string> errors;

    ClassesBuilder tt_builder;
    auto ret = tt_builder.build(parser_);
    if (!ret.has_value()) {
        return unexpected(ret.error());
    }

    Classes ast = std::move(ret.value());

    // check for undefined classes
    if (!errors.empty()) {
        return unexpected(errors);
    }

    // check inheritance loops
    for (const auto &error : checkLoops(ast)) {
        errors.push_back(error);
    }

    // if (!errors.empty()) {
    //     return unexpected(errors);
    // }

    // collect features
    parser_->reset();
    for (const auto &error : MethodCollector().collect_methods(parser_, &ast)) {
        errors.push_back(error);
    }

    // if (!errors.empty()) {
    //     return unexpected(errors);
    // }

    // check overwrite correctness
    for (const auto &error : checkOverwrites(ast)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // typecheck
    parser_->reset();
    for (const auto &error : TypeChecker().check(parser_)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // return the typed AST
    return std::move(ast);
}
