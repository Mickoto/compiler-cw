#ifndef SEMANTICS_PASSES_EXPR_COLLECTOR_H_
#define SEMANTICS_PASSES_EXPR_COLLECTOR_H_

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include "Classes.h"

class ExprCollector : public CoolParserBaseVisitor {
  private:
    // define any necessary fields
    std::vector<std::string> errors;
    std::stack<std::unique_ptr<Expr>> scratchpad_;
    Classes *ast;
    CoolLexer *lexer_;
    Type current;
    std::deque<std::unordered_map<std::string, Type>> scopes;
    std::unordered_set<std::string> visitedMethods;
    std::unordered_set<std::string> visitedAttrs;

    Type find_id_type(std::string& identifier);
    Methods* find_nearest_methods_containing(Type t, std::string& method);
    std::unique_ptr<Expr> buildExpr(CoolParser::ExprContext *ctx);
    void scope_attributes();

  public:
    virtual std::any visitClass(CoolParser::ClassContext *ctx) override;
    virtual std::any visitMethod(CoolParser::MethodContext *ctx) override;
    virtual std::any visitAttr(CoolParser::AttrContext *ctx) override;

    virtual std::any visitExpr(CoolParser::ExprContext *ctx) override;
    void visitMemDispatch(CoolParser::ExprContext *ctx);
    void visitDispatch(CoolParser::ExprContext *ctx);
    virtual std::any visitIf(CoolParser::IfContext *ctx) override;
    virtual std::any visitWhile(CoolParser::WhileContext *ctx) override;
    virtual std::any visitBlock(CoolParser::BlockContext *ctx) override;
    virtual std::any visitLet(CoolParser::LetContext *ctx) override;
    virtual std::any visitCase(CoolParser::CaseContext *ctx) override;
    void visitUnop(CoolParser::ExprContext *ctx);
    void visitNew(CoolParser::ExprContext *ctx);
    void visitIsVoid(CoolParser::ExprContext *ctx);
    void visitNeg(CoolParser::ExprContext *ctx);
    void visitNot(CoolParser::ExprContext *ctx);
    void visitBinop(CoolParser::ExprContext *ctx);
    void visitArithmetic(CoolParser::ExprContext *ctx);
    void visitComparison(CoolParser::ExprContext *ctx);
    void visitEq(CoolParser::ExprContext *ctx);
    void visitAssign(CoolParser::ExprContext *ctx);
    virtual std::any visitObject(CoolParser::ObjectContext *ctx) override;
    virtual std::any visitInteger(CoolParser::IntegerContext *ctx) override;
    virtual std::any visitBool(CoolParser::BoolContext *ctx) override;
    virtual std::any visitString(CoolParser::StringContext *ctx) override;

    std::vector<std::string> check(CoolLexer *lexer, CoolParser *parser, Classes *ast);
};

#endif
