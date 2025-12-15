#ifndef SEMANTICS_PASSES_CLASS_CHECKER_H_
#define SEMANTICS_PASSES_CLASS_CHECKER_H_

#include <any>
#include <expected>
#include <string>
#include <map>
#include <vector>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "semantics/typed-ast/Classes.h"

class ClassesBuilder : public CoolParserBaseVisitor {
private:
    std::vector<std::string> errors;

    Classes ast;
    Type obj;
    std::vector<std::pair<Type, std::string>> supers;

public:
    ClassesBuilder() {}

    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;

    std::expected<Classes, std::vector<std::string>> build(CoolParser *parser);
};

std::vector<std::string> checkLoops(Classes &tree);
std::vector<std::string> checkOverwrites(Classes &tree);

#endif //SEMANTICS_PASSES_CLASS_CHECKER_H_
