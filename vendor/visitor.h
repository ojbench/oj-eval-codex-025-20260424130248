#pragma once

#include "lang.h"

template <typename T>
class Visitor {
 public:
  virtual T visitProgram(Program *node) { return {}; }
  virtual T visitFunctionDeclaration(FunctionDeclaration *node) { return {}; }

  virtual T visitStatement(Statement *node) {
    if (node->is<ExpressionStatement>()) {
      return visitExpressionStatement(node->as<ExpressionStatement>());
    } else if (node->is<SetStatement>()) {
      return visitSetStatement(node->as<SetStatement>());
    } else if (node->is<IfStatement>()) {
      return visitIfStatement(node->as<IfStatement>());
    } else if (node->is<ForStatement>()) {
      return visitForStatement(node->as<ForStatement>());
    } else if (node->is<BlockStatement>()) {
      return visitBlockStatement(node->as<BlockStatement>());
    } else if (node->is<ReturnStatement>()) {
      return visitReturnStatement(node->as<ReturnStatement>());
    }
    throw SyntaxError(node, "Unknown type");
  }
  virtual T visitExpressionStatement(ExpressionStatement *node) {
    return visitExpression(node->expr);
  }
  virtual T visitSetStatement(SetStatement *node) { return {}; }
  virtual T visitIfStatement(IfStatement *node) { return {}; }
  virtual T visitForStatement(ForStatement *node) { return {}; }
  virtual T visitBlockStatement(BlockStatement *node) { return {}; }
  virtual T visitReturnStatement(ReturnStatement *node) { return {}; }

  virtual T visitExpression(Expression *node) {
    if (node->is<IntegerLiteral>()) {
      return visitIntegerLiteral(node->as<IntegerLiteral>());
    } else if (node->is<Variable>()) {
      return visitVariable(node->as<Variable>());
    } else if (node->is<CallExpression>()) {
      return visitCallExpression(node->as<CallExpression>());
    }
    throw SyntaxError(node, "Unknown type");
  }
  virtual T visitIntegerLiteral(IntegerLiteral *node) { return {}; }
  virtual T visitVariable(Variable *node) { return {}; }
  virtual T visitCallExpression(CallExpression *node) { return {}; }
};
