#include "ModifyLLVM.h"
#include "SearchC.h"
#include "Controller.h"

int main(int argc, char **argv){
    string path = argv[1];

    // TODO 多线程
    Controller controller(path);
    controller.Entry();

    return 0;
}
