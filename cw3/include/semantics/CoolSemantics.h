#ifndef SEMANTICS_COOL_SEMANTICS_H_
#define SEMANTICS_COOL_SEMANTICS_H_

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include "semantics/typed-ast/Classes.h"
#include "CoolLexer.h"
#include "CoolParser.h"

class CoolSemantics {
  private:
    CoolLexer *lexer_;
    CoolParser *parser_;

  public:
    CoolSemantics(CoolLexer *lexer, CoolParser *parser)
        : lexer_(lexer), parser_(parser) {}

    // Runs semantic analysis and returns the typed AST generated in the
    // process.
    //
    // In case of errors, a list of error messages is returned.
    std::expected<std::unique_ptr<Classes>, std::vector<std::string>> run();
};

#endif
