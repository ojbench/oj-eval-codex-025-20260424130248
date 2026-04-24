// Unified solution: supports cheat and anticheat based on input shape.
// - If only one program appears before EOF: run cheat transform and output.
// - If two programs then extra input: run anticheat heuristic and output score.

#include <iostream>
#include <sstream>
#include <string>

#include "lang.h"
#include "transform.h"
#include "visitor.h"

// Minimal cheat transform: rename all variables with a prefix
class CheatTransform : public Transform {
 public:
  Variable *transformVariable(Variable *node) override {
    return new Variable("ppca_" + node->name);
  }
};

// Simple visitor to compute a rough length metric (similar to baseline)
class LengthVisitor : public Visitor<int> {
 public:
  int visitProgram(Program *node) override {
    int l = 0;
    for (auto func : node->body) l += visitFunctionDeclaration(func);
    return l;
  }
  int visitFunctionDeclaration(FunctionDeclaration *node) override {
    return visitStatement(node->body);
  }
  int visitExpressionStatement(ExpressionStatement *node) override {
    return visitExpression(node->expr) + 1;
  }
  int visitSetStatement(SetStatement *node) override {
    return visitExpression(node->value) + 1;
  }
  int visitIfStatement(IfStatement *node) override {
    return visitExpression(node->condition) + visitStatement(node->body) + 1;
  }
  int visitForStatement(ForStatement *node) override {
    return visitStatement(node->body) + visitExpression(node->test) + visitStatement(node->update) + visitStatement(node->body) + 1;
  }
  int visitBlockStatement(BlockStatement *node) override {
    int l = 0;
    for (auto stmt : node->body) l += visitStatement(stmt);
    return l;
  }
  int visitReturnStatement(ReturnStatement *node) override { return 1; }
  int visitIntegerLiteral(IntegerLiteral *node) override { return 1; }
  int visitVariable(Variable *node) override { return 1; }
  int visitCallExpression(CallExpression *node) override {
    int l = 1;
    for (auto expr : node->args) l += visitExpression(expr);
    return l;
  }
};

static int programLength(Program *p) { return LengthVisitor().visitProgram(p); }

int main() {
  // We need to peek input to decide mode. scanProgram consumes until endprogram.
  // Try to read first program; if fails, just exit.
  try {
    Program *prog1 = scanProgram(std::cin);

    // After reading first program, try reading second program. We have to lookahead:
    // If next non-whitespace begins with '(' or identifier tokens, scanProgram should succeed;
    // Otherwise, treat as cheat mode.

    // Buffer the remainder so we can decide. Read the rest of stdin into a string.
    std::string remainder;
    {
      std::ostringstream tmp;
      tmp << std::cin.rdbuf();
      remainder = tmp.str();
    }

    // Create an istringstream from remainder to attempt reading a second program.
    std::istringstream rem(remainder);

    // Try to scan a second program safely. If scanProgram throws, fallback to cheat mode.
    Program *prog2 = nullptr;
    bool haveSecond = false;
    try {
      // If remainder is empty or only whitespace, there is likely no second program.
      bool only_ws = true;
      for (char c : remainder) {
        if (!std::isspace(static_cast<unsigned char>(c))) { only_ws = false; break; }
      }
      if (!only_ws) {
        prog2 = scanProgram(rem);
        haveSecond = true;
      }
    } catch (const EvalError &) {
      haveSecond = false;
      prog2 = nullptr;
    }

    if (!haveSecond) {
      // CHEAT MODE: transform the single program and output.
      auto cheat = CheatTransform().transformProgram(prog1);
      std::cout << cheat->toString();
      return 0;
    }

    // ANTICHEAT MODE: remainder after second program is the input to run.
    std::string inputRest;
    {
      std::ostringstream tmp;
      tmp << rem.rdbuf();
      inputRest = tmp.str();
    }

    // Optionally simulate execution of prog1 to capture output (not used here, but aligns with baseline)
    std::istringstream iss(inputRest);
    std::ostringstream oss;
    prog1->eval(1000000, iss, oss);

    // Simple similarity score based on structural length.
    int len1 = programLength(prog1);
    int len2 = programLength(prog2);
    int diff = std::abs(len1 - len2);
    double s;
    if (diff < 20) {
      s = 0.5 + 0.5 * (1.0 - diff / 20.0);
    } else {
      s = 0.5;
    }
    if (s < 0) s = 0; if (s > 1) s = 1;
    std::cout << s << std::endl;
    return 0;
  } catch (const EvalError &e) {
    // Per spec, runtime errors are treated as output 0 for anticheat; for cheat, ensure we fail gracefully.
    std::cerr << e.what() << std::endl;
    std::cout << 0 << std::endl;
    return 0;
  }
}
