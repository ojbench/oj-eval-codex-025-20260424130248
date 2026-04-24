#pragma once

#include "lang.h"

class Transform {
 public:
  virtual Program *transformProgram(Program *node) {
    std::vector<FunctionDeclaration *> body;
    for (auto decl : node->body) {
      body.push_back(transformFunctionDeclaration(decl));
    }
    return new Program(body);
  }

  virtual FunctionDeclaration *transformFunctionDeclaration(FunctionDeclaration *node) {
    std::vector<Variable *> params;
    for (auto param : node->params) {
      params.push_back(transformVariable(param));
    }
    return new FunctionDeclaration(node->name, params, transformStatement(node->body));
  }

  virtual Statement *transformStatement(Statement *node) {
    if (node->is<ExpressionStatement>()) {
      return transformExpressionStatement(node->as<ExpressionStatement>());
    } else if (node->is<SetStatement>()) {
      return transformSetStatement(node->as<SetStatement>());
    } else if (node->is<IfStatement>()) {
      return transformIfStatement(node->as<IfStatement>());
    } else if (node->is<ForStatement>()) {
      return transformForStatement(node->as<ForStatement>());
    } else if (node->is<BlockStatement>()) {
      return transformBlockStatement(node->as<BlockStatement>());
    } else if (node->is<ReturnStatement>()) {
      return transformReturnStatement(node->as<ReturnStatement>());
    }
    throw SyntaxError(node, "Unknown type");
  }

  virtual Statement *transformExpressionStatement(ExpressionStatement *node) {
    return new ExpressionStatement(transformExpression(node->expr));
  }

  virtual Statement *transformSetStatement(SetStatement *node) {
    return new SetStatement(transformVariable(node->name), transformExpression(node->value));
  }

  virtual Statement *transformIfStatement(IfStatement *node) {
    return new IfStatement(transformExpression(node->condition), transformStatement(node->body));
  }

  virtual Statement *transformForStatement(ForStatement *node) {
    return new ForStatement(transformStatement(node->init), transformExpression(node->test),
                            transformStatement(node->update), transformStatement(node->body));
  }

  virtual Statement *transformBlockStatement(BlockStatement *node) {
    std::vector<Statement *> body;
    for (auto stmt : node->body) {
      body.push_back(transformStatement(stmt));
    }
    return new BlockStatement(body);
  }

  virtual Statement *transformReturnStatement(ReturnStatement *node) {
    return new ReturnStatement(transformExpression(node->value));
  }

  virtual Expression *transformExpression(Expression *node) {
    if (node->is<IntegerLiteral>()) {
      return transformIntegerLiteral(node->as<IntegerLiteral>());
    } else if (node->is<Variable>()) {
      return transformVariable(node->as<Variable>());
    } else if (node->is<CallExpression>()) {
      return transformCallExpression(node->as<CallExpression>());
    }
    throw SyntaxError(node, "Unknown type");
  }

  virtual Expression *transformIntegerLiteral(IntegerLiteral *node) {
    return new IntegerLiteral(node->value);
  }

  virtual Variable *transformVariable(Variable *node) {
    return new Variable(node->name);
  }

  virtual Expression *transformCallExpression(CallExpression *node) {
    std::vector<Expression *> args;
    for (auto arg : node->args) {
      args.push_back(transformExpression(arg));
    }
    return new CallExpression(node->func, args);
  }
};
