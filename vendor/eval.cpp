#include <fstream>
#include <iostream>

#include "lang.h"

int main (int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [source-file]" << std::endl;
    return 2;
  }
  try {
    auto code = std::ifstream(argv[1]);
    if (!code) {
      std::cerr << "Source file '" << argv[1] << "' not found" << std::endl;
      return 2;
    }
    auto *p = scanProgram(code);
    // std::cout << p->toString();
    int time = p->eval(1000000);
    std::cout << "Evaluation took " << time << " ticks to complete" << std::endl;
  } catch (const EvalError &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}
