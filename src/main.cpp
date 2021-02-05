//
// Created by xxxx on xxxx/xx/xx.
//
#include "Controller.h"

static void parseArguments(int argc, char **argv) {

}


int main(int argc, char **argv, char **envp)  {
  parseArguments(argc, argv);

  string path = argv[1];

  Controller controller(path);
  controller.Entry();

  return 0;
}
