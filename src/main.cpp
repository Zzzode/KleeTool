//
// Created by xxxx on xxxx/xx/xx.
//
#include "CmdLine.h"
#include "Controller.h"

using namespace llvm;

namespace {
// main positional opts
cl::opt<string>
    InputPath(cl::Positional, cl::desc("<input path>"), cl::init("-"));
cl::opt<string>
    OutputPath(cl::Positional, cl::desc("<output path>"), cl::init("-"));

/*** Startup options ***/
cl::OptionCategory
    TerminationCat("Startup option",
                   "This option control termination of the overall KleeTool "
                   "execution and of individual states.");
cl::opt<std::string>
    MaxTime("max-time",
            cl::desc("Halt execution after the specified duration.  "
                     "Set to 0s to disable (default=0s)"),
            cl::init("0s"),
            cl::cat(TerminationCat));
}  // namespace

int main(int argc, char** argv, char** envp) {
  cl::ParseCommandLineOptions(argc, argv, " KleeTool for EXGEN. \n");

  CmdLine::HideOptions(llvm::cl::GeneralCategory);

  //  string path = argv[1];
  //
  //  Controller controller(path);
  //  controller.Entry();

  return 0;
}
