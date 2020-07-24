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

class ArithOp{
public:
    void Init(const smatch& instRexRes){
        arithOpRex = R"((\w+)\.(\d+))";

        smatch opRexRes;
        op = instRexRes[2];
        opType = instRexRes[3];
        res.first = instRexRes[1];
        lName.first = instRexRes[4];
        rName.first = instRexRes[5];

        if (regex_search(res.first, opRexRes, arithOpRex)) {
            res.first = opRexRes[1];
            res.second = stoi(opRexRes[2]);
        }

        if (regex_search(lName.first, opRexRes, arithOpRex)) {
            lName.first = opRexRes[1];
            lName.second = stoi(opRexRes[2]);
        }

        if (regex_search(rName.first, opRexRes, arithOpRex)) {
            rName.first = opRexRes[1];
            rName.second = stoi(opRexRes[2]);
        }
    }

    string GetString() {
        string _res;
        if (res.second == 0)
            _res += res.first;
        else
            _res += res.first + "." + to_string(res.second);

        _res += " = " + op + " " + opType + " ";

        if (lName.second == 0)
            _res += lName.first;
        else
            _res += lName.first + "." + to_string(lName.second);

        _res += ", ";

        if (rName.second == 0)
            _res += rName.first;
        else
            _res += rName.first + "." + to_string(rName.second);

        return _res;
    }

private:
    regex arithOpRex;

    string op;
    string opType;

    pair<string, int> res;
    pair<string, int> lName;
    pair<string, int> rName;
};

class FuncCall{
public:
    void Init(const smatch& instRexRes){
        ;
    }

    string GetString() {;}

private:
    string funcType;
    string funcName;
    vector<pair<string, string>> callArgs;
};

class StoreInst{
public:
    void Init(const smatch& instRexRes){
        ;
    }

    string GetString(){
        ;
    }

private:
};

class Instruction {
public:
    Instruction() {
        instType = 0;
        arithOp = new ArithOp;
        funcCall = new FuncCall;
        storeInst = new StoreInst;

        arithInstRex = R"(\%\"(.*)\" = (\w+) (\w+) \%\"(.*)\", \%\"(.*)\")";
        funcCallRex = R"(\%\"(FunctionCall)\" = (call) (i256) @\"(\w+)\"\(((\w+)\s(.*)[,\s]*)*\))";
        storeRex = R"(store (\w+) %\"(\w+)\", (\w+\**) @\"(\w+)\")";
    }

    void InitInst(const string &_inst) {
        smatch instRexRes;
        if (regex_search(_inst, instRexRes, arithInstRex)) {
            arithOp->Init(instRexRes);
            instType = 1;
        } else if (regex_search(_inst, instRexRes, funcCallRex)){
            funcCall->Init(instRexRes);
            instType = 2;
        } else if (regex_search(_inst, instRexRes, storeRex)){
            storeInst->Init(instRexRes);
            instType = 3;
        }
    }

    string GetString() const{
        switch (instType) {
            case 1: return arithOp->GetString();
            case 2: return funcCall->GetString();
            case 3: return storeInst->GetString();
            default: return "No such inst type!";
        }
    }

    string GetType() const {
        switch (instType) {
            case 1: return "Arithmetic operation";
            case 2: return "Function call";
            case 3: return "Store operation";
            default: return "Unknown type";
        }
    }

private:
    // 1:arith 2: func call 3: store
    int instType;

    ArithOp *arithOp;
    FuncCall *funcCall;
    StoreInst *storeInst;

private:
    regex arithInstRex;
    regex funcCallRex;
    regex storeRex;
};

class Function {
public:
    Function() : instLength(0) {}

    string GetFuncName() { return funcName; }

    bool SetFuncName(string _name) {
        funcName = std::move(_name);
        return true;
    }

    Instruction GetInst(int _index) {
        return instructions[_index];
    }

    void InitInsts(unsigned int _len) {
        instructions.resize(_len);
        instLength = _len;
    }

    unsigned int GetInstNum() const {
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
