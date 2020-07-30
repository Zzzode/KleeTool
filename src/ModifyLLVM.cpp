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

vector<string> ModifyLLVM::ModifyArithInst(ArithOp *_inst, int _num, LLVMFunction &_llFunction) {
    int opType = _inst->GetOp() == "add" ? 1 : 2;
    // 首先添加klee_assume
    // 除非这是第一个函数 否则并没有必要符号化
    // TODO 全局变量需要符号化
    for (int i = 0; i < 3; ++i) {
        _llFunction.AddAssume(opType, _num++, _inst->GetReg(i + 1));
    }
    // _llFunction.Show();
    // cout << endl;

    vector<string> funcLines = _llFunction.GetNewLines();

    for (int i = 0; i < funcLines.size(); ++i) {
        string funcLine = funcLines[i];
        if (funcLine.find(_inst->GetString()) != string::npos) {
            // cout << "debug: " << _inst->GetString() << endl;

            vector<string> newStr = _llFunction.GetAssumeStr();
            // _llFunction.ShowAssume();
            funcLines.insert(funcLines.begin() + i + 1, newStr.begin(), newStr.end());
        }
    }

    return funcLines;
}

/**
 * TODO
 * @param _inst
 * @param _llFunction
 * @return
 */
vector<string> ModifyLLVM::ModifyCallInst(FuncCall *_inst, LLVMFunction &_llFunction) {
    vector<string> funcLines = _llFunction.GetNewLines();
    int argNum = _inst->GetArgNum();
    // _llFunction.Show();

    for (int i = 0; i < funcLines.size(); ++i) {
        string funcLine = funcLines[i];
        if (funcLine.find(_inst->GetString()) != string::npos) { ;
        }
    }

    return vector<string>();
}

vector<string> ModifyLLVM::ModifyStoreInst(StoreInst *_inst, LLVMFunction &_llFunction) {
    vector<string> funcLines = _llFunction.GetNewLines();
    // _llFunction.Show();
    for (int i = 0; i < funcLines.size(); ++i) {
        string funcLine = funcLines[i];
        if (funcLine.find(_inst->GetString()) != string::npos) {
            string newStr = R"(  call void @klee_make_symbolic(i8* bitcast )";
            newStr += "(" + _inst->GetDest()->GetString() + " to i8*), i64 4, i8* getelementptr inbounds ([";
            unsigned int _size = _inst->GetDest()->GetPureName().size() + 1;
            newStr += to_string(_size) + " x i8], [" + to_string(_size) + " x i8]* @.str";
            newStr += llvmFile->symCount == 0 ? "" : "." + to_string(llvmFile->symCount);
            newStr += ", i64 0, i64 0))";

            llvmFile->AddGlobalSymDecl(_inst->GetDest());

            funcLines.insert(funcLines.begin() + i + 1, newStr);
            // cout << "debug: " << newStr << endl;
        }
    }
    // cout << "debug: " << _llFunction.symCount << endl;
    return funcLines;
}

