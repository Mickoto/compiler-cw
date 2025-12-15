#include "CoolSemantics.h"

#include <expected>
#include <string>
#include <vector>

#include "passes/ClassChecker.h"
#include "passes/TypeChecker.h"

using namespace std;

// Runs semantic analysis and returns a list of errors, if any.
//
// TODO: change the type from void * to your typed AST type
expected<void *, vector<string>> CoolSemantics::run() {
    vector<string> errors;

    ClassesBuilder tt_builder;
    auto ret = tt_builder.build(parser_);
    if (!ret.has_value()) {
        return unexpected(ret.error());
    }

    Classes ast = ret.value();

    // check for undefined classes
    if (!errors.empty()) {
        return unexpected(errors);
    }

    // check inheritance loops
    for (const auto &error : checkLoops(ast)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // collect features

    // check methods are overridden correctly

    parser_->reset();
    for (const auto &error : TypeChecker().check(parser_)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    // return the typed AST
    return nullptr;
}
