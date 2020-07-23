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

namespace ModifyLLVM {
    using namespace std;

    vector<string> split(const string &str, const string &delim);

    vector<pair<string, set<vector<string>>>> parseJson(const string &thisPath,
                                                        const string &folderName);

//    void GetFiles(const string &newPath, vector<string> &fileNames);

    void modifyLLVM(const string &newPath, vector<string> fileLines,
                    const pair<string, set<vector<string>>> &func);

    void configArgv(const string &tmpArgv);

    bool threadControl(const string &thisPath, const string &folderName);

} // namespace ModifyLLVM

#endif // KLEE_MODIFYLLVM_H
