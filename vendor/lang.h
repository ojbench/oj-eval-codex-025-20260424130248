#pragma once

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// +------------------------------------+
// |              Values                |
// +------------------------------------+

class Value {
 public:
  virtual ~Value() {}
};
using ValuePtr = std::shared_ptr<Value>;

class IntValue : public Value {
 public:
  int value;
  IntValue(int value) : value(value) {}
};

class ArrayValue : public Value {
 public:
  int length;
  int *contents;
  ArrayValue(int length);
  ~ArrayValue() override;
};


// +------------------------------------+
// |        Program Structures          |
// +------------------------------------+

class Program;
struct Context;

class BaseObject {
 public:
  virtual ~BaseObject() {}
  virtual std::string toString() const = 0;

  template <typename T>
  bool is() const {
    return dynamic_cast<const T *>(this) != nullptr;
  }
  template <typename T>
  T *as() {
    return dynamic_cast<T *>(this);
  }
};


// +------------------------------------+
// |           Expressions              |
// +------------------------------------+

class Expression : public BaseObject {
 public:
  virtual ValuePtr eval(Context &ctx) const = 0;
};

class IntegerLiteral : public Expression {
 public:
  int value;

  IntegerLiteral(int value) : value(value) {}
  std::string toString() const override;
  ValuePtr eval(Context &ctx) const override;
};

class Variable : public Expression {
 public:
  std::string name;

  Variable(std::string name) : name(std::move(name)) {}
  std::string toString() const override;
  ValuePtr eval(Context &ctx) const override;
};

class CallExpression : public Expression {
 public:
  std::string func;
  std::vector<Expression *> args;

  CallExpression(std::string func, std::vector<Expression *> args)
      : func(std::move(func)), args(std::move(args)) {}
  std::string toString() const override;
  ValuePtr eval(Context &ctx) const override;
};


// +------------------------------------+
// |           Statements               |
// +------------------------------------+

class Statement : public BaseObject {
 public:
  virtual void eval(Context &ctx) const = 0;
};

class ExpressionStatement : public Statement {
 public:
  Expression *expr;

  ExpressionStatement(Expression *expr) : expr(expr) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class SetStatement : public Statement {
 public:
  Variable *name;
  Expression *value;

  SetStatement(Variable *name, Expression *value) : name(name), value(value) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class IfStatement : public Statement {
 public:
  Expression *condition;
  Statement *body;

  IfStatement(Expression *condition, Statement *body)
      : condition(condition), body(body) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class ForStatement : public Statement {
 public:
  Statement *init;
  Expression *test;
  Statement *update;
  Statement *body;

  ForStatement(Statement *init, Expression *test, Statement *update,
               Statement *body)
      : init(init), test(test), update(update), body(body) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class BlockStatement : public Statement {
 public:
  std::vector<Statement *> body;

  BlockStatement(std::vector<Statement *> body) : body(std::move(body)) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class ReturnStatement : public Statement {
 public:
  Expression *value;

  ReturnStatement(Expression *value) : value(value) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};


// +------------------------------------+
// |        Global Constructs           |
// +------------------------------------+

class FunctionDeclaration : public BaseObject {
 public:
  std::string name;
  std::vector<Variable *> params;
  Statement *body;

  FunctionDeclaration(std::string name, std::vector<Variable *> params,
                      Statement *body)
      : name(std::move(name)), params(std::move(params)), body(body) {}
  std::string toString() const override;
};

class Program : public BaseObject {
 public:
  std::vector<FunctionDeclaration *> body;
  std::unordered_map<std::string, FunctionDeclaration *> index;

  Program(std::vector<FunctionDeclaration *> body);
  std::string toString() const override;
  int eval(int timeLimit, std::istream &is = std::cin, std::ostream &os = std::cout);
};


// +------------------------------------+
// |            Exceptions              |
// +------------------------------------+

class EvalError : public std::exception {
 public:
  const BaseObject *location;
  std::string reason;

  EvalError(const BaseObject *location, const std::string &reason_)
      : location(location) {
    if (location == nullptr) {
      reason = reason_;
      return;
    }
    reason = "At " + location->toString() + ":\n" + reason_;
  }
  const char *what() const noexcept override { return reason.c_str(); }
};
class SyntaxError : public EvalError {
 public:
  SyntaxError(const BaseObject *location, const std::string &reason)
      : EvalError(location, "Syntax error: " + reason) {}
};
class RuntimeError : public EvalError {
 public:
  RuntimeError(const BaseObject *location, const std::string &reason)
      : EvalError(location, "Runtime error: " + reason) {}
};


// +------------------------------------+
// |               Parser               |
// +------------------------------------+

BaseObject *scan(std::istream &is);
Program *scanProgram(std::istream &is);

// +------------------------------------+
// |   Variables and Helper Functions   |
// +------------------------------------+

extern const int kIdMaxLength;
extern const std::unordered_set<std::string> keywords;
extern const std::unordered_set<std::string> builtinFunctions;

bool isTruthy(const BaseObject *ctx, ValuePtr value);
std::string indent(const std::string &s);
bool isValidIdentifier(const std::string &name);
void removeWhitespaces(std::istream &is);
void expectClosingParens(std::istream &is);
std::string scanToken(std::istream &is);
std::string scanIdentifier(std::istream &is);
