#include <any>
#include <string>
#include <vector>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "semantics/typed-ast/Classes.h"

class MethodCollector : public CoolParserBaseVisitor {
private:
    std::vector<std::string> errors;

    Classes *ast;
    Type current;

public:
    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;
    virtual std::any visitMethod(CoolParser::MethodContext *ctx) override;
    virtual std::any visitAttr(CoolParser::AttrContext *ctx) override;

    std::vector<std::string> collect_methods(CoolParser *parser, Classes *classes);
};

std::vector<std::string> checkOverwrites(Classes &classes);
