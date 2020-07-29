//
// Created by zode on 2020/7/13.
//

#ifndef MODIFYLLVM_SEARCHC_H
#define MODIFYLLVM_SEARCHC_H

#include <utility>
#include <vector>
#include <string>

using namespace std;

class SearchC {
public:
    explicit SearchC(string func);
public:
    string funName;
    vector<string> globalVar;
    vector<string> funParams;
    vector<string> localVar;
};


#endif //MODIFYLLVM_SEARCHC_H
