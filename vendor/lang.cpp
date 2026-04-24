#include "lang.h"

#include <cctype>
#include <iostream>
#include <stack>
#include <type_traits>
#include <unordered_set>

const int kIdMaxLength = 255;
const std::unordered_set<std::string> keywords = {
    "set", "if", "for", "block", "return", "function",
};
const std::unordered_set<std::string> builtinFunctions = {
    "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "||", "&&", "!",
    "scan", "print", "array.create", "array.get", "array.set", "array.scan", "array.print",
};

bool isTruthy(const BaseObject *ctx, ValuePtr value) {
  auto iv = std::dynamic_pointer_cast<IntValue>(value);
  if (iv == nullptr) {
    throw RuntimeError(ctx, "Type error: if condition should be an int");
  }
  return iv->value != 0;
}

ArrayValue::ArrayValue(int length) : length(length) {
  if (length > 1000000) throw RuntimeError(nullptr, "Out of memory");
  contents = new int[length];
  for (int i = 0; i < length; ++i) contents[i] = 0;
}
ArrayValue::~ArrayValue() { delete[] contents; }

struct VariableSet {
  std::unordered_map<std::string, ValuePtr> values;
  ValuePtr getOrThrow(const BaseObject *ctx, const std::string &name) {
    if (values.count(name) == 0) {
      throw RuntimeError(ctx, "Use of undefined variable: " + name);
    }
    if (values[name] == nullptr) {
      throw RuntimeError(ctx, "Use of uninitialized variable: " + name);
    }
    return values[name];
  }
};

struct Context {
  std::istream &is;
  std::ostream &os;
  std::stack<VariableSet> callStack;
  Program *program;
  int timeLeft;

  VariableSet &currentFrame() { return callStack.top(); }
  ValuePtr getOrThrow(const BaseObject *ctx, const std::string &name) {
    return currentFrame().getOrThrow(ctx, name);
  }
  void set(const BaseObject *ctx, const std::string &name, ValuePtr value) {
    if (program->index.count(name) > 0 || builtinFunctions.count(name) > 0) {
      throw RuntimeError(ctx, "Assigning to function " + name);
    }
    currentFrame().values[name] = value;
  }
  void tick() {
    --timeLeft;
    if (timeLeft < 0) {
      throw RuntimeError(nullptr, "Time limit exceeded");
    }
  }
};

std::string IntegerLiteral::toString() const { return std::to_string(value); }
ValuePtr IntegerLiteral::eval(Context &ctx) const {
  ctx.tick();
  return std::make_shared<IntValue>(value);
}

std::string Variable::toString() const { return name; }
ValuePtr Variable::eval(Context &ctx) const {
  ctx.tick();
  return ctx.getOrThrow(this, name);
}

std::string ExpressionStatement::toString() const { return expr->toString(); }
void ExpressionStatement::eval(Context &ctx) const { expr->eval(ctx); }

struct ReturnFromCall {
  ValuePtr value;
};
std::string CallExpression::toString() const {
  std::string str = std::string("(") + func;
  for (const auto &arg : args) {
    str += " ";
    str += arg->toString();
  }
  str += ")";
  return str;
}
ValuePtr CallExpression::eval(Context &ctx) const {
  ctx.tick();
  std::vector<ValuePtr> argValues;
  for (const auto &arg : args) {
    argValues.push_back(arg->eval(ctx));
  }

  auto requireArity = [&](int arity) {
    if (args.size() != arity) {
      throw RuntimeError(this, "Function arity mismatch at " + func);
    }
  };
  auto readInt = [&](int ix) {
    auto iv = std::dynamic_pointer_cast<IntValue>(argValues[ix]);
    if (!iv) throw RuntimeError(this, "Type error: int expected");
    return iv->value;
  };

  if (func == "+") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x + y);
  } else if (func == "-") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x - y);
  } else if (func == "*") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x * y);
  } else if (func == "/") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    if (y == 0) {
      throw RuntimeError(this, "Divide by zero");
    }
    return std::make_shared<IntValue>(x / y);
  } else if (func == "%") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    if (y == 0) {
      throw RuntimeError(this, "Mod by zero");
    }
    return std::make_shared<IntValue>(x % y);
  } else if (func == "<") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x < y);
  } else if (func == ">") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x > y);
  } else if (func == "<=") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x <= y);
  } else if (func == ">=") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x >= y);
  } else if (func == "==") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x == y);
  } else if (func == "!=") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x != y);
  } else if (func == "||") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x || y);
  } else if (func == "&&") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x && y);
  } else if (func == "!") {
    requireArity(1);
    int x = readInt(0);
    return std::make_shared<IntValue>(!x);
  } else if (func == "scan") {
    requireArity(0);
    int x;
    ctx.is >> x;
    return std::make_shared<IntValue>(x);
  } else if (func == "print") {
    requireArity(1);
    ctx.os << readInt(0) << '\n';
    return std::make_shared<IntValue>(0);
  } else if (func == "array.create") {
    requireArity(1);
    return std::make_shared<ArrayValue>(readInt(0));
  } else if (func == "array.scan") {
    requireArity(1);
    int length = readInt(0);
    auto array = std::make_shared<ArrayValue>(length);
    for (int i = 0; i < length; ++i) {
      ctx.is >> array->contents[i];
    }
    return array;
  } else if (func == "array.print") {
    requireArity(1);
    auto array = std::dynamic_pointer_cast<ArrayValue>(argValues[0]);
    if (!array)
      throw RuntimeError(this, "Type error at array.get: array expected");
    for (int i = 0; i < array->length; ++i) {
      ctx.os << array->contents[i] << std::endl;
    }
    return std::make_shared<IntValue>(0);
  } else if (func == "array.get") {
    requireArity(2);
    auto array = std::dynamic_pointer_cast<ArrayValue>(argValues[0]);
    int index = readInt(1);
    if (!array)
      throw RuntimeError(this, "Type error at array.get: array expected");
    if (index >= array->length || index < 0) {
      throw RuntimeError(this, "Index out of bounds at array.get");
    }
    return std::make_shared<IntValue>(array->contents[index]);
  } else if (func == "array.set") {
    requireArity(3);
    auto array = std::dynamic_pointer_cast<ArrayValue>(argValues[0]);
    int index = readInt(1);
    int value = readInt(2);
    if (!array)
      throw RuntimeError(this, "Type error at array.set: array expected");
    if (index >= array->length || index < 0) {
      throw RuntimeError(this, "Index out of bounds at array.set");
    }
    array->contents[index] = value;
    return std::make_shared<IntValue>(0);
  }

  auto *funcObject = ctx.program->index[func];
  if (!funcObject) throw RuntimeError(this, "No such function: " + func);
  requireArity(funcObject->params.size());

  ctx.callStack.push({});
  for (int i = 0; i < args.size(); ++i) {
    const auto &name = funcObject->params[i];
    if (ctx.program->index.count(name->name) > 0) {
      throw RuntimeError(
          this, "Function parameter name is global identifier: " + name->name);
    }
    ctx.set(this, name->name, argValues[i]);
  }

  try {
    funcObject->body->eval(ctx);
  } catch (ReturnFromCall r) {
    ctx.callStack.pop();
    return r.value;
  }

  ctx.callStack.pop();
  return std::make_shared<IntValue>(0);
}

std::string indent(const std::string &s) {
  std::string res = "  ";
  for (char ch : s) {
    res += ch;
    if (ch == '\n') {
      res += "  ";
    }
  }
  return res;
}

std::string SetStatement::toString() const {
  return std::string("(set ") + name->toString() + " " + value->toString() + ")";
}
void SetStatement::eval(Context &ctx) const {
  ctx.tick();
  ctx.set(this, name->name, value->eval(ctx));
}

std::string IfStatement::toString() const {
  return std::string("(if ") + condition->toString() + "\n" +
         indent(body->toString()) + ")";
}
void IfStatement::eval(Context &ctx) const {
  ctx.tick();
  bool ok = isTruthy(this, condition->eval(ctx));
  if (ok) {
    body->eval(ctx);
  }
}

std::string ForStatement::toString() const {
  return std::string("(for\n") + indent(init->toString()) + "\n" +
         indent(test->toString()) + "\n" + indent(update->toString()) + "\n" +
         indent(body->toString()) + ")";
}
void ForStatement::eval(Context &ctx) const {
  ctx.tick();
  for (init->eval(ctx); isTruthy(this, test->eval(ctx)); update->eval(ctx)) {
    body->eval(ctx);
  }
}

std::string BlockStatement::toString() const {
  std::string str = "(block";
  for (const auto &stmt : body) {
    str += "\n";
    str += indent(stmt->toString());
  }
  str += ")";
  return str;
}
void BlockStatement::eval(Context &ctx) const {
  for (auto stmt : body) {
    stmt->eval(ctx);
  }
}

std::string ReturnStatement::toString() const {
  return std::string("(return ") + value->toString() + ")";
}
void ReturnStatement::eval(Context &ctx) const {
  throw ReturnFromCall{value->eval(ctx)};
}

std::string FunctionDeclaration::toString() const {
  std::string str = "(function (";
  str += name;
  for (const auto &param : params) {
    str += " ";
    str += param->toString();
  }
  str += ")\n";
  str += indent(body->toString());
  str += ")";
  return str;
}

Program::Program(std::vector<FunctionDeclaration *> body)
    : body(std::move(body)) {
  for (auto el : this->body) {
    const auto &name = el->name;
    if (builtinFunctions.count(name) > 0) {
      throw SyntaxError(nullptr, "Redefining built-in function: " + name);
    }
    if (index.count(name) > 0) {
      throw SyntaxError(nullptr, "Duplicate function declaration: " + name);
    }
    index[name] = el;
  }
}
std::string Program::toString() const {
  std::string str;
  for (auto el : body) {
    str += el->toString();
    str += "\n\n";
  }
  return str;
}
int Program::eval(int timeLimit, std::istream &is, std::ostream &os) {
  Context ctx{
      .is = is,
      .os = os,
      .callStack = {},
      .program = this,
      .timeLeft = timeLimit,
  };
  CallExpression("main", {}).eval(ctx);
  return timeLimit - ctx.timeLeft;
}

bool isValidIdentifier(const std::string &name) {
  if (name.length() > kIdMaxLength) return false;
  if (name.empty()) return false;
  if (isdigit(name[0])) return false;
  if (name[0] == '-') {
    if (name.length() == 1) return true;
    bool isNumber = true;
    for (int i = 1; i < name.length(); ++i) {
      if (!isdigit(name[i])) {
        isNumber = false;
        break;
      }
    }
    if (isNumber) return false;
  }
  for (char ch : name) {
    if (ch == ')' || ch == '(' || ch == ';') return false;
    if (!isgraph(ch)) return false;
  }
  if (keywords.count(name) > 0) return false;
  return true;
}

void removeWhitespaces(std::istream &is) {
  while (is && isspace(is.peek())) is.get();
  if (is.peek() == ';') {
    int ch;
    do {
      ch = is.get();
    } while (ch != EOF && ch != '\n');
    removeWhitespaces(is);
  }
}

void expectClosingParens(std::istream &is) {
  removeWhitespaces(is);
  int ch = is.get();
  if (ch != ')') {
    throw SyntaxError(
        nullptr, std::string("Closing parenthesis expected, got ") + char(ch));
  }
}

std::string scanToken(std::istream &is) {
  removeWhitespaces(is);
  std::string token;
  for (int next = is.peek(); !isspace(next) && next != ')' && next != ';';
       next = is.peek()) {
    token += is.get();
  }
  return token;
}

std::string scanIdentifier(std::istream &is) {
  auto name = scanToken(is);
  if (!isValidIdentifier(name))
    throw SyntaxError(nullptr, "Invalid identifier: " + name);
  return name;
}

template <typename T>
static T *scanT(std::istream &is) {
  auto *construct = scan(is);
  if (construct == nullptr) throw SyntaxError(nullptr, "Unexpected EOF");
  if (!construct->is<T>()) {
    if constexpr (std::is_same_v<T, Statement>) {
      if (construct->is<Expression>()) {
        return new ExpressionStatement(construct->as<Expression>());
      }
    }
    throw SyntaxError(nullptr, std::string("Wrong construct type; ") +
                                   typeid(*construct).name() + " found, " +
                                   typeid(T).name() + " expected");
  }
  return construct->as<T>();
}

BaseObject *scan(std::istream &is) {
  // ignore whitespaces
  removeWhitespaces(is);
  if (!is || is.peek() == EOF) return nullptr;
  if (is.peek() != '(') {
    // variable or literal
    auto name = scanToken(is);
    if (name.empty()) return nullptr;
    if (name[0] == '-') {
      bool isLiteral = true;
      if (name.length() == 1) {
        isLiteral = false;
      } else {
        for (char ch : name.substr(1)) {
          if (!isdigit(ch)) {
            isLiteral = false;
            break;
          }
        }
      }
      if (isLiteral) {
        int value = std::stoi(name);
        return new IntegerLiteral(value);
      }
    }
    if (isdigit(name[0])) {
      for (char ch : name) {
        if (!isdigit(ch)) {
          throw SyntaxError(nullptr, "Invalid literal: " + name);
        }
      }
      int value = std::stoi(name);
      return new IntegerLiteral(value);
    }
    if (isValidIdentifier(name)) {
      return new Variable(name);
    }
    throw SyntaxError(nullptr, "Invalid identifier: " + name);
  }
  is.get();

  auto type = scanToken(is);
  if (type == "set") {
    auto name = scanIdentifier(is);
    auto *value = scanT<Expression>(is);
    expectClosingParens(is);
    return new SetStatement(new Variable(name), value->as<Expression>());
  } else if (type == "if") {
    auto *cond = scanT<Expression>(is);
    auto *body = scanT<Statement>(is);
    expectClosingParens(is);
    return new IfStatement(cond, body);
  } else if (type == "for") {
    auto *init = scanT<Statement>(is);
    auto *test = scanT<Expression>(is);
    auto *update = scanT<Statement>(is);
    auto *body = scanT<Statement>(is);
    expectClosingParens(is);
    return new ForStatement(init, test, update, body);
  } else if (type == "block") {
    std::vector<Statement *> body;
    removeWhitespaces(is);
    while (is.peek() != ')') {
      body.push_back(scanT<Statement>(is));
      removeWhitespaces(is);
    }
    expectClosingParens(is);
    return new BlockStatement(body);
  } else if (type == "return") {
    auto *value = scanT<Expression>(is);
    expectClosingParens(is);
    return new ReturnStatement(value);
  } else if (type == "function") {
    removeWhitespaces(is);
    if (is.get() != '(') {
      throw SyntaxError(nullptr, "Opening parenthesis expected");
    }
    auto name = scanIdentifier(is);
    std::vector<Variable *> params;
    removeWhitespaces(is);
    while (is.peek() != ')') {
      params.push_back(new Variable(scanIdentifier(is)));
      removeWhitespaces(is);
    }
    expectClosingParens(is);
    auto *body = scanT<Statement>(is);
    expectClosingParens(is);
    return new FunctionDeclaration(name, params, body);
  } else {
    // call expression
    auto &name = type;
    if (!isValidIdentifier(name))
      throw SyntaxError(nullptr, "Invalid identifier: " + name);
    std::vector<Expression *> args;
    removeWhitespaces(is);
    while (is.peek() != ')') {
      args.push_back(scanT<Expression>(is));
      removeWhitespaces(is);
    }
    expectClosingParens(is);
    return new CallExpression(name, args);
  }
}

Program *scanProgram(std::istream &is) {
  std::vector<FunctionDeclaration *> body;
  while (true) {
    auto *el = scan(is);
    if (el == nullptr) break;
    if (!el->is<FunctionDeclaration>()) {
      if (el->is<Variable>() && el->as<Variable>()->name == "endprogram") {
        break;
      }
      throw SyntaxError(nullptr, "Invalid program element");
    }
    body.push_back(el->as<FunctionDeclaration>());
  }
  return new Program(body);
}
