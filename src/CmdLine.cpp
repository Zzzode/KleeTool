//
// Created by zode on 2021/2/6.
//

#include "CmdLine.h"

#include "Version.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

void CmdLine::HideOptions(llvm::cl::OptionCategory &Category) {
  StringMap<cl::Option *> &map = cl::getRegisteredOptions();

  for (auto &elem : map) {
#if LLVM_VERSION_CODE >= LLVM_VERSION(9, 0)
    for (auto &cat : elem.second->Categories) {
#else
    {
      auto &cat = elem.second->Category;
#endif
      if (cat == &Category) {
        elem.second->setHiddenFlag(cl::Hidden);
      }
    }
  }
}
void CmdLine::HideUnrelatedOptions(cl::OptionCategory& Category) {
  // wait for the child to finish
}
