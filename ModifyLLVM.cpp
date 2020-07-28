//
// Created by zode on 2020/6/29.
//
#include "ModifyLLVM.h"
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>


void ModifyLLVM::Modify(const string& instStr) {
    vector<string>::iterator iter;
}

void ModifyLLVM::ModifyArithInst(ArithOp *inst, int num) {
    int opType = inst->GetOp() == "add" ? 1 : 2;
    // 首先添加klee_assume
    // 除非这是第一个函数 否则并没有必要符号化
    // 全局变量需要符号化
    for (int i = 0; i < 3; ++i) {
        kleeAssume.emplace_back(opType, num++, inst->GetReg(i + 1));

        // kleeAssume[i].Show();
    }
    // cout << endl;
}
