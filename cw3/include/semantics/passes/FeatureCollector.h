#ifndef SEMANTICS_PASSES_FEATURE_COLLECTOR_H_
#define SEMANTICS_PASSES_FEATURE_COLLECTOR_H_

#include <any>
#include <string>
#include <vector>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "Classes.h"

class FeatureCollector : public CoolParserBaseVisitor {
private:
    std::vector<std::string> errors;

    Classes *ast;
    Type current;
    bool fatal_;

    void initialize_base_classes();
public:
    FeatureCollector() : fatal_(false) {}

    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;
    virtual std::any visitMethod(CoolParser::MethodContext *ctx) override;
    virtual std::any visitAttr(CoolParser::AttrContext *ctx) override;

    std::vector<std::string> collect_methods(CoolParser *parser, Classes *classes);
    bool fatal() const { return fatal_; };
};

#endif
