#include <iostream>

#include "lang.h"
#include "transform.h"

class Cheat : public Transform {
 public:
  Variable *transformVariable(Variable *node) override {
    return new Variable("ppca-" + node->name);
  }
};

int main() {
  auto code = scanProgram(std::cin);
  auto cheat = Cheat().transformProgram(code);
  std::cout << cheat->toString();
  return 0;
}
