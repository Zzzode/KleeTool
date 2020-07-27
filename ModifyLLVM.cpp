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


/**
 * Create and Modify llvm ir file
 * @param newPath  folder path
 * @param llName ll ir file name
 * @return success OR fail
 */
void ModifyLLVM::Modify(const string &newPath, vector<string> fileLines, const pair<string, set<vector<string> > > &func) {
    // file stream
    ofstream outLLfile;

    // measure all Functions
    // each fun should be measured independent
    vector<string> globalDeclares;
    vector<string> symbolicLines;

    if (!func.first.empty()) {
        // configure llvm lines
        int gCount = 0; // count global declares
        set<string> vars;
        for (auto instructions : func.second) {
            regex varReg_1(" *" + instructions[3] + R"( = load i32, i32\* (@\w+).*)");
            regex varReg_2(" *" + instructions[4] + R"( = load i32, i32\* (@\w+).*)");
            regex resReg(R"( *store i32 )" + instructions[0] +
                         R"(, i32\* ([@%]\w+).*)");
            smatch loadResult;
            string varName_1, varName_2, resName;

            // 这边需要先确定.ll文件中的最初声明位置
            bool inFunBlock = false;
            for (const auto &fileLine : fileLines) {
                if (inFunBlock && fileLine.find('}') != string::npos) {
                    inFunBlock = false;
                    break;
                }
                if (!inFunBlock) {
                    regex funDeclarePattern(".*@" + func.first + R"(.*\{)");
                    smatch regFunDeclareResult;
                    inFunBlock =
                            regex_match(fileLine, regFunDeclareResult, funDeclarePattern);
                    continue;
                }

                varName_1 =
                        regex_match(fileLine, loadResult, varReg_1) && varName_1.empty()
                        ? loadResult[1].str()
                        : varName_1;
                varName_2 =
                        regex_match(fileLine, loadResult, varReg_2) && varName_2.empty()
                        ? loadResult[1].str()
                        : varName_2;
                resName = regex_match(fileLine, loadResult, resReg) && resName.empty()
                          ? loadResult[1].str()
                          : resName;
            }
            // ! 我注释了算数操作的结果的符号化 但运行时可能需要
            // vars.push_back(resName);
            if (!varName_1.empty())
                vars.insert(varName_1);
            if (!varName_2.empty())
                vars.insert(varName_2);
        }
        // ! 非全局变量的命名为数字
        // FIXME 暂时不符号化函数内变量
        if (!vars.empty()) {
            for (auto &var : vars) {
                if (var.empty())
                    continue;
                string tmpGol = R"(@.str)";
                string tmpSym = R"(call void @klee_make_symbolic(i8* bitcast (i32* )" +
                                var +
                                R"( to i8*), i64 4, i8* getelementptr inbounds ([)" +
                                to_string(var.size()) + R"( x i8], [)" +
                                to_string(var.size()) + R"( x i8]* @.str)";
                if (gCount == 0) {
                    tmpGol += " = private unnamed_addr constant [" +
                              to_string(var.size()) + R"( x i8] c")" +
                              var.substr(1, var.size() - 1) + R"(\00", align 1)";
                    tmpSym += R"(, i64 0, i64 0)))";
                } else {
                    tmpGol += "." + to_string(gCount) +
                              " = private unnamed_addr constant [" +
                              to_string(var.size()) + R"( x i8] c")" +
                              var.substr(1, var.size() - 1) + R"(\00", align 1)";
                    tmpSym += "." + to_string(gCount) + R"(, i64 0, i64 0)))";
                }

                globalDeclares.push_back(tmpGol);
                symbolicLines.push_back(tmpSym);
                gCount++;
            }

            globalDeclares.emplace_back(
                    "declare void @klee_make_symbolic(i8*, i64, i8*)");

            // FIXME 局部变量无法使用`bitcast (%struct.str* @global to i8*)`
            bool inGlobalDeclare = false;
            regex funDeclarePattern(R"(define.*@.*\{)");
            regex thisFunPattern(".*@" + func.first + R"(.*\{)");
            smatch regFunDeclareResult;
            smatch regThisFunResult;
            for (int i = 0; i < fileLines.size(); i++) {
                string fileLine = fileLines[i];
                if (fileLine.find(R"(target triple = "x86_64-pc-linux-gnu")") !=
                    string::npos ||
                    fileLine.find(R"(target triple = "x86_64-unknown-linux-gnu")") !=
                    string::npos) {
                    inGlobalDeclare = true;
                    continue;
                }
                if (inGlobalDeclare &&
                    fileLine.find("; Function Attrs:") != string::npos) {
                    for (int j = 0; j < globalDeclares.size(); j++) {
                        fileLines.insert(fileLines.begin() + i + j - 1, globalDeclares[j]);
                    }
                    i += globalDeclares.size();
                    inGlobalDeclare = false;
                    continue;
                }

                if (!inGlobalDeclare &&
                    regex_match(fileLine, regFunDeclareResult, thisFunPattern)) {
                    for (int j = 0; j < symbolicLines.size(); ++j) {
                        fileLines.insert(fileLines.begin() + i + j + 1,
                                         "  " + symbolicLines[j]);
                    }
                    i += symbolicLines.size();
                    break;
                }
            }
        }
    }

    // output .ll file
    outLLfile.open(newPath + "/" + func.first + ".ll");
    for (auto fileLine : fileLines)
        outLLfile << fileLine << endl;
    outLLfile.close();
    cout << "Modify LLVM OK!" << endl;
}

void ModifyLLVM::ModifyArithInst(ArithOp *inst, int num) {
    int opType = inst->GetOp() == "add" ? 1 : 2;
    vector<KleeAssume> kleeAssume;

    // 首先添加klee_assume
    // 除非并没有必要符号化
    for (int i = 0; i < 3; ++i) {
        kleeAssume.emplace_back(opType, num++, inst->GetReg(i + 1));

        kleeAssume[i].Show();
    }
    cout << endl;
}
