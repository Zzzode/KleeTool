//
// Created by zode on 2020/6/29.
//
#ifndef KLEE_MODIFYLLVM_H
#define KLEE_MODIFYLLVM_H

#include <cstring>
#include <set>
#include <string>
#include <vector>
#include <thread>

#include "Controller.h"

using namespace std;

/**
 * @brief 只有算数指令需要klee_assume()
 */
class KleeAssume {
//    %11 = load i32, i32* %3
//    %12 = icmp sgt i32 %11, 0
//    %13 = zext i1 %12 to i32
//    %14 = sext i32 %13 to i64
//    call void @klee_assume(i64 %14)
public:
    KleeAssume() = default;
    // 1:Add 2:sub
    KleeAssume(int _opType, int _num, RegName *_nameL) {
        count = 1;
        string tmpStr;
        string tmpRes;
        string tmpOp;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        tmpOp = "load i256 " + _nameL->GetName();
        tmpStr = tmpRes;
        newStr.push_back(tmpRes + " = " + tmpOp);
        count++;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        if (_opType == 1)
            tmpOp = "icmp sgt i256 " + tmpStr + ", 0";
        else if(_opType == 2)
            tmpOp = "icmp slt i256 " + tmpStr + ", 0";
        tmpStr = tmpRes;
        newStr.push_back(tmpRes + " = " + tmpOp);
        count++;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        tmpOp = "zext i1 " + tmpStr + " to i32";
        tmpStr = tmpRes;
        newStr.push_back(tmpRes + " = " + tmpOp);
        count++;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        tmpOp = "sext i32 " + tmpStr + " to i64";
        tmpStr = tmpRes;
        newStr.push_back(tmpRes + " = " + tmpOp);

        newStr.push_back("call void @klee_assume(i64 " + tmpStr + ")");
    }

    void Show(){
        for(string line : newStr)
            cout << line << endl;
    }

private:
    int count;
    vector<string> newStr;
};

class GlobalSymDecl{
public:
    GlobalSymDecl()= default;
    void Init() {

    }
private:
    string symName;
    pair<string, int> gDecl;
};

class ModifyLLVM {
public:
    ModifyLLVM() {
        kleeSymDecl = "declare void @klee_make_symbolic(i8*, i64, i8*)";
        kleeAssumeDecl = "declare void @klee_assume(i64)";
    }

    void Modify(const string &newPath, vector<string> fileLines,
                    const pair<string, set<vector<string>>> &func);

    void ModifyArithInst(ArithOp *inst, int num);

private:
    string kleeSymDecl;
    string kleeAssumeDecl;

    string globalSymDecl;
};



void configArgv(const string &tmpArgv);

#endif // KLEE_MODIFYLLVM_H
