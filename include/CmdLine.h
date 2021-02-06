//
// Created by zode on 2021/2/6.
//

#ifndef TEST_COMMANDSOLVER_H
#define TEST_COMMANDSOLVER_H

#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace std;


class CmdLine {
 public:
  /// Hide all options in the specified category
  static void HideOptions(llvm::cl::OptionCategory& Category);

  /// Hide all options except the ones in the specified category
  static void HideUnrelatedOptions(llvm::cl::OptionCategory& Category);
};

#endif  // TEST_COMMANDSOLVER_H
