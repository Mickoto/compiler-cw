#ifndef SEMANTICS_PASSES_CLASS_COLLECTOR_H_
#define SEMANTICS_PASSES_CLASS_COLLECTOR_H_

#include <any>
#include <expected>
#include <string>
#include <map>
#include <vector>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "semantics/typed-ast/Classes.h"

class ClassCollector : public CoolParserBaseVisitor {
private:
    std::vector<std::string> errors;

    Classes *ast;
    Type obj;
    std::vector<std::pair<Type, std::string>> supers;

    void instantiate_base_classes();
    void link_inheritance();
public:
    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;

    std::vector<std::string> collect(CoolParser *parser, Classes *ast);
};

#endif
