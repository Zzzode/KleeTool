// Created by zode on 2020/7/16.
//

#ifndef MODIFYLLVM_CONTROLLER_H
#define MODIFYLLVM_CONTROLLER_H

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <set>
#include <thread>
#include <mutex>

using namespace std;

class Instruction{
public:
    Instruction() {
        instRegex = R"(\%\"(.*)\" = (\w+) (\w+) \%\"(.*)\", \%\"(.*)\")";
        opRex = R"((\w+)\.(\d+))"
    }

    void InitInst(const string& _inst){
        smatch resRex;
        if(!regex_search(_inst, resRex, instRegex)) {
            cout << "Match inst failed." << endl;
            return;
        }

        res = resRex[1];
        op = resRex[2];
        opType = resRex[3];
        lName = resRex[4];
        rName = resRex[5];
    }

    string GetString(){
        return res + " = " + op + " " + opType + " " + lName + ", " + rName;
    }

private:
    regex instRegex;
    regex opRex;

    string op;
    string opType;

    pair<string, int> res;
    pair<string, int> lName;
    pair<string, int> rName;
};

class Function {
public:
    Function() = default;

    string GetFuncName() { return funcName; }

    bool SetFuncName(string _name){
        funcName = std::move(_name);
        return true;
    }

    Instruction GetInst(int _index){
        return instructions[_index];
    }

    void InitInsts(unsigned int _len){
        instructions.resize(_len);
        instLength = _len;
    }

    unsigned int GetInstNum() const{
        return instLength;
    }

private:
    unsigned int instLength;

    string funcName;

    vector<Instruction> instructions;
};

class FuncChain {
public:
    FuncChain() = default;

    unsigned int GetChainLength() const {
        return length;
    }

    void InitFunc(int _len) {
        length = _len;
        function = new Function[_len];
    }

    Function GetFunction(int _index) {
        return function[_index];
    }

private:
    unsigned int length;

    Function *function;
};

class FuncChains {
public:
    unsigned int GetLength() const { return length; }

    void InitChains(uint _len) {
        length = _len;
        funcChain = new FuncChain[_len];
    }

    FuncChain GetChain(int _index) {
        return funcChain[_index];
    }

private:
    unsigned int length;

    FuncChain *funcChain;
};

class Controller {
public:
    explicit Controller(string _path) : path(std::move(_path)) {
        funcChains = new FuncChains;
        threadCount = 10;
    }

    void Entry();

    void GetFiles();

    void ParseJson(const string &folderName);

    void FunChains();

    void ThreadControllor();

private:

    string path;

    vector<string> folderNames;

    string jsonName;

    rapidjson::Document document;

    FuncChains *funcChains;

private:
    mutex threadMutex;

    int threadCount;
};


#endif //MODIFYLLVM_CONTROLLER_H
