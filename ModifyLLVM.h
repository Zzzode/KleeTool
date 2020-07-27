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
//    %11 = load i32, i32* %3, align 4
//    %12 = icmp sgt i32 %11, 0
//    %13 = zext i1 %12 to i32
//    %14 = sext i32 %13 to i64
//    call void @klee_assume(i64 %14)
public:
    // 1:Add 2:sub
    KleeAssume(int _type, RegName _nameL, RegName _nameR, RegName _nameRes){
        count = 1;
        string tmpStr;
        if (_type == 1) {
//            tmpStr =  "%\"tmpRes" + ('.' + to_string(count) + "\"") + " = load " + _nameL.GetType().substr(0, _nameL.GetType().size() - 1) + ", " + _nameL.GetType() + " " + ;
            newStr.push_back(tmpStr);
        } else if (_type == 2) {
            ;
        }
    };

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

private:
    string kleeSymDecl;
    string kleeAssumeDecl;

    string globalSymDecl;
};

void modifyLLVM(const string &newPath, vector<string> fileLines,
                const pair<string, set<vector<string>>> &func);

void configArgv(const string &tmpArgv);

#endif // KLEE_MODIFYLLVM_H
