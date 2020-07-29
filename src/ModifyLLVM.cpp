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


//void ModifyLLVM::Modify(const string& _instStr, LLVMFunction _llFunction) {
//    vector<string> funcLines = _llFunction.GetNewLines();
//
////    cout << "debug: " << _instStr << endl;
//
//    for(int i = 0; i < funcLines.size(); ++i) {
//        string funcLine = funcLines[i];
//        if (funcLine.find(_instStr) != string::npos){
//            cout << "debug: " << _instStr << endl;
//
//            vector<string> newStr = _llFunction.GetAssumeStr();
//            _llFunction.Show();
//            funcLines.insert(funcLines.begin() + i + 1, newStr.begin(), newStr.end());
//        }
//    }
//
//    _llFunction.WriteNewLines(funcLines);
//
//    exit(0);
//}

vector<string> ModifyLLVM::ModifyArithInst(ArithOp *inst, int num, LLVMFunction _llFunction) {
    int opType = inst->GetOp() == "add" ? 1 : 2;
    // 首先添加klee_assume
    // 除非这是第一个函数 否则并没有必要符号化
    // 全局变量需要符号化
    for (int i = 0; i < 3; ++i) {
        _llFunction.AddAssume(opType, num++, inst->GetReg(i + 1));
    }
//    _llFunction.Show();
    // cout << endl;
//    Modify(inst->GetString(), _llFunction);

    vector<string> funcLines = _llFunction.GetNewLines();
//    cout << "debug: " << _instStr << endl;

    for(int i = 0; i < funcLines.size(); ++i) {
        string funcLine = funcLines[i];
        if (funcLine.find(inst->GetString()) != string::npos){
            cout << "debug: " << inst->GetString() << endl;

            vector<string> newStr = _llFunction.GetAssumeStr();
            _llFunction.ShowAssume();
            funcLines.insert(funcLines.begin() + i + 1, newStr.begin(), newStr.end());
        }
    }

    return funcLines;
}
