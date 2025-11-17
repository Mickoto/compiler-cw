#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

using namespace std;
using namespace antlr4;

namespace fs = filesystem;

/// Converts token to coursework-specific string representation.
string cool_token_to_string(CoolLexer *lexer, Token *token) {
    auto token_type = token->getType();

    // clang-format off
    switch (token_type) {
        case static_cast<size_t>(-1) : return "EOF";

        case CoolLexer::SEMI   : return "';'";
        case CoolLexer::LCURLY : return "'{'";
        case CoolLexer::RCURLY : return "'}'";
        case CoolLexer::LPARR : return "'('";
        case CoolLexer::COMMA  : return "','";
        case CoolLexer::RPARR : return "')'";
        case CoolLexer::COLON  : return "':'";
        case CoolLexer::AT     : return "'@'";
        case CoolLexer::DOT    : return "'.'";
        case CoolLexer::PLUS   : return "'+'";
        case CoolLexer::MINUS  : return "'-'";
        case CoolLexer::TIMES   : return "'*'";
        case CoolLexer::DIV  : return "'/'";
        case CoolLexer::COMPL  : return "'~'";
        case CoolLexer::LT     : return "'<'";
        case CoolLexer::EQU     : return "'='";

        case CoolLexer::DARROW : return "DARROW";
        case CoolLexer::ASSIGN : return "ASSIGN";
        case CoolLexer::LE     : return "LE";

        case CoolLexer::CLASS  : return "CLASS";
        case CoolLexer::ELSE   : return "ELSE";
        case CoolLexer::FI     : return "FI";
        case CoolLexer::IF     : return "IF";
        case CoolLexer::IN     : return "IN";
        case CoolLexer::INHERITS : return "INHERITS";
        case CoolLexer::ISVOID : return "ISVOID";
        case CoolLexer::LET    : return "LET";
        case CoolLexer::LOOP   : return "LOOP";
        case CoolLexer::POOL   : return "POOL";
        case CoolLexer::THEN   : return "THEN";
        case CoolLexer::WHILE  : return "WHILE";
        case CoolLexer::CASE   : return "CASE";
        case CoolLexer::ESAC   : return "ESAC";
        case CoolLexer::NEW    : return "NEW";
        case CoolLexer::OF     : return "OF";
        case CoolLexer::NOT    : return "NOT";

        case CoolLexer::BOOL_CONST : return "BOOL_CONST";
        case CoolLexer::INT_CONST  : return "INT_CONST = " + token->getText();
        case CoolLexer::STR_CONST  : return "STR_CONST";
        case CoolLexer::TYPEID     : return "TYPEID = " + token->getText();
        case CoolLexer::OBJECTID   : return "OBJECTID = " + token->getText();
        case CoolLexer::ERROR      : return "ERROR";

        default : return "<Invalid Token>: " + token->toString();
    }
    // clang-format on
}

class TreePrinter : public CoolParserBaseVisitor {
  private:
    CoolLexer *lexer_;
    CoolParser *parser_;
    string file_name_;
    int indent_level;

    std::string indent() {
        std::stringstream ss;
        for (int i = 0; i < indent_level; i++) {
            ss << ' ';
        }
        return ss.str();
    }

    void printNoExpr(int line) {
        cout << indent() << '#' << line << endl;
        cout << indent() << "_no_expr" << endl;
        cout << indent() << ": _no_type" << endl;
    }

    void printSelfExpr(int line) {
        cout << indent() << '#' << line << endl;
        cout << indent() << "_object" << endl;
        cout << indent() << "  self" << endl;
        cout << indent() << ": _no_type" << endl;
    }

  public:
    TreePrinter(CoolLexer *lexer, CoolParser *parser, const string &file_name)
        : lexer_(lexer), parser_(parser), file_name_(file_name) {}

    any visitProgram(CoolParser::ProgramContext *ctx) override {
        cout << '#' << ctx->getStop()->getLine() << endl;
        cout << "_program" << endl;
        indent_level = 2;
        visitChildren(ctx);

        return any{};
    }

    any visitClass(CoolParser::ClassContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_class" << endl;
        indent_level+=2;
        cout << indent() << ctx->classname->getText() << endl;
        if (ctx->inherit) {
            cout << indent() << ctx->inherit->getText() << endl;
        }
        else {
            cout << indent() << "Object" << endl;
        }
        cout << indent() << '"' << file_name_ << '"' << endl;
        cout << indent() << '(' << endl;

        visitChildren(ctx);

        cout << indent() << ')' << endl;
        indent_level -= 2;

        return any{};
    }

    any visitType(CoolParser::TypeContext *ctx) override {
        if (ctx->TYPEID()) {
            cout << indent() << ctx->TYPEID()->getText() << endl;
        }
        else {
            cout << indent() << "SELF_TYPE" << endl;
        }

        return any{};
    }

    any visitDefine(CoolParser::DefineContext *ctx) override {
        cout << indent() << ctx->OBJECTID()->getText() << endl;
        visitType(ctx->type());

        return any{};
    }

    any visitFormal(CoolParser::FormalContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_formal" << endl;
        indent_level += 2;
        visitDefine(ctx->define());
        indent_level -= 2;

        return any{};
    }

    any visitMethod(CoolParser::MethodContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_method" << endl;
        indent_level += 2;
        cout << indent() << ctx->name->getText() << endl;
        for (auto formal : ctx->formal()) {
            visitFormal(formal);
        }
        visitType(ctx->type());
        visitExpr(ctx->body);
        indent_level -= 2;

        return any{};
    }

    any visitAttr(CoolParser::AttrContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_attr" << endl;
        indent_level += 2;
        visitDefine(ctx->define());
        if (ctx->expr()) {
            visitExpr(ctx->expr());
        }
        else {
            printNoExpr(ctx->getStop()->getLine());
        }
        indent_level -= 2;

        return any{};
    }

    any visitExpr(CoolParser::ExprContext *ctx) override {
        if (ctx->obj) {
            visitMemDispatch(ctx);
        } else if (ctx->name) {
            visitDispatch(ctx);
        } else if (ctx->unop) {
            visitUnop(ctx);
        } else if (ctx->binop) {
            visitBinop(ctx);
        } else if (ctx->ASSIGN()) {
            visitAssign(ctx);
        } else {
            visitChildren(ctx);
        }
        return any{};
    }


    any visitMemDispatch(CoolParser::ExprContext *ctx) {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        if (ctx->type())
            cout << indent() << "_static_dispatch" << endl;
        else
            cout << indent() << "_dispatch" << endl;

        indent_level += 2;
        visitExpr(ctx->obj);
        if (ctx->type())
            visitType(ctx->type());
        cout << indent() << ctx->name->getText() << endl;
        cout << indent() << '(' << endl;
        for (auto arg : ctx->args) {
            visitExpr(arg);
        }
        cout << indent() << ')' << endl;
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitDispatch(CoolParser::ExprContext *ctx) {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_dispatch" << endl;

        indent_level += 2;
        printSelfExpr(ctx->getStop()->getLine());
        cout << indent() << ctx->name->getText() << endl;
        cout << indent() << '(' << endl;
        for (auto arg : ctx->args) {
            visitExpr(arg);
        }
        cout << indent() << ')' << endl;
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitIf(CoolParser::IfContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_cond" << endl;

        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitWhile(CoolParser::WhileContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_loop" << endl;

        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitBlock(CoolParser::BlockContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_block" << endl;

        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitLetdef(CoolParser::LetdefContext *ctx) override {
        visitDefine(ctx->define());
        if (ctx->expr()) {
            visitExpr(ctx->expr());
        }
        else {
            printNoExpr(ctx->getStop()->getLine());
        }

        return any{};
    }

    any visitLet(CoolParser::LetContext *ctx) override {
        for (auto let : ctx->letdef()) {
            cout << indent() << '#' << ctx->getStop()->getLine() << endl;
            cout << indent() << "_let" << endl;
            indent_level += 2;
            visitLetdef(let);
        }
        visitExpr(ctx->expr());
        for (auto let : ctx->letdef()) {
            indent_level -= 2;
            cout << indent() << ": _no_type" << endl;
        }

        return any{};
    }

    any visitBranch(CoolParser::BranchContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_branch" << endl;
        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;

        return any{};
    }

    any visitCase(CoolParser::CaseContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_typcase" << endl;
        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitUnop(CoolParser::ExprContext *ctx) {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        if (ctx->NEW()) {
            cout << indent() << "_new" << endl;
        } else if (ctx->COMPL()) {
            cout << indent() << "_neg" << endl;
        } else if (ctx->ISVOID()) {
            cout << indent() << "_isvoid" << endl;
        } else if (ctx->NOT()) {
            cout << indent() << "_comp" << endl;
        }
        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitBinop(CoolParser::ExprContext *ctx) {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        if (ctx->TIMES()) {
            cout << indent() << "_mul" << endl;
        } else if (ctx->DIV()) {
            cout << indent() << "_divide" << endl;
        } else if (ctx->PLUS()) {
            cout << indent() << "_plus" << endl;
        } else if (ctx->MINUS()) {
            cout << indent() << "_sub" << endl;
        } else if (ctx->LE()) {
            cout << indent() << "_le" << endl;
        } else if (ctx->LT()) {
            cout << indent() << "_lt" << endl;
        } else if (ctx->EQU()) {
            cout << indent() << "_eq" << endl;
        }

        indent_level += 2;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitAssign(CoolParser::ExprContext *ctx) {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_assign" << endl;

        indent_level += 2;
        cout << indent() << ctx->OBJECTID()->getText() << endl;
        visitChildren(ctx);
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitObject(CoolParser::ObjectContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_object" << endl;
        indent_level += 2;
        if (ctx->OBJECTID()) {
            cout << indent() << ctx->OBJECTID()->getText() << endl;
        }
        else {
            cout << indent() << "self" << endl;
        }
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitInteger(CoolParser::IntegerContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_int" << endl;
        indent_level += 2;
        cout << indent() << ctx->INT_CONST()->getText() << endl;
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitBool(CoolParser::BoolContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_bool" << endl;
        indent_level += 2;
        int char_index = ctx->BOOL_CONST()->getSymbol()->getStartIndex();
        cout << indent() << lexer_->get_bool_value(char_index) << endl;
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

    any visitString(CoolParser::StringContext *ctx) override {
        cout << indent() << '#' << ctx->getStop()->getLine() << endl;
        cout << indent() << "_string" << endl;
        indent_level += 2;
        int char_index = ctx->STR_CONST()->getSymbol()->getStartIndex();
        cout << indent() <<
            '"' << lexer_->get_string_pretty(char_index) << '"'
            << endl;
        indent_level -= 2;
        cout << indent() << ": _no_type" << endl;

        return any{};
    }

  public:
    void print(CoolParser::ProgramContext *ctx) { visitProgram(ctx); }
};

class ErrorPrinter : public BaseErrorListener {
  private:
    string file_name_;
    CoolLexer *lexer_;
    bool has_error_ = false;

  public:
    ErrorPrinter(const string &file_name, CoolLexer *lexer)
        : file_name_(file_name), lexer_(lexer) {}

    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg,
                             std::exception_ptr e) override {
        has_error_ = true;
        cout << '"' << file_name_ << "\", line " << line
             << ": syntax error at or near "
             << cool_token_to_string(lexer_, offendingSymbol) << endl;
    }

    bool has_error() const { return has_error_; }
};

class NoAssocChecker : public CoolParserBaseVisitor {
  private:
    bool inNoAssoc;
    CoolParser *parser_;
    ErrorPrinter *listener_;

    public:
    NoAssocChecker(CoolParser *parser, ErrorPrinter *listener) :
            inNoAssoc(false), parser_(parser), listener_(listener) {}

    any visitExpr(CoolParser::ExprContext *ctx) override {
        if (ctx->binop && (ctx->LE() || ctx->LT() || ctx->EQU()) ) {
            if (inNoAssoc) {
                listener_->syntaxError(NULL, ctx->binop,
                                       ctx->getStop()->getLine(),
                                       0, "", NULL);
            }
            else {
                inNoAssoc = true;
                visitExpr(ctx->expr(1));
            }
        }
        else {
            for (auto expr : ctx->expr()) {
                inNoAssoc = false;
                visitExpr(expr);
            }
        }
        return any{};
    }

    void check(CoolParser::ProgramContext *ctx) { visitProgram(ctx); }
};

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        cerr << "Expecting exactly one argument: name of input file" << endl;
        return 1;
    }

    auto file_path = argv[1];
    ifstream fin(file_path);

    auto file_name = fs::path(file_path).filename().string();

    ANTLRInputStream input(fin);
    CoolLexer lexer(&input);

    CommonTokenStream tokenStream(&lexer);

    CoolParser parser(&tokenStream);

    ErrorPrinter error_printer(file_name, &lexer);

    parser.removeErrorListener(&ConsoleErrorListener::INSTANCE);
    parser.addErrorListener(&error_printer);

    // This will trigger the error_printer, in case there are errors.
    parser.program();
    parser.reset();

    if (!error_printer.has_error()) {
        auto ctx = parser.program();
        NoAssocChecker(&parser, &error_printer).check(ctx);

        if (!error_printer.has_error()) {
            TreePrinter(&lexer, &parser, file_name).print(ctx);
        } else {
            cout << "Compilation halted due to lex and parse errors" << endl;
        }
    } else {
        cout << "Compilation halted due to lex and parse errors" << endl;
    }

    return 0;
}
