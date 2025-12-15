#ifndef SEMANTICS_TYPED_AST_EXPR_H_
#define SEMANTICS_TYPED_AST_EXPR_H_

typedef int Type;

class Expr {
  private:
    Type type_;

  public:
    Expr(Type type) : type_(type) {}
    virtual ~Expr() = default;

    Type get_type() const { return type_; }
};

#endif
