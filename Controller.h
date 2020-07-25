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

class RegName {
public:
    RegName() : count(0) {
        arithOpRex = R"((\w+)\.(\d+))";
    }

    RegName(string _type, string _name, int _count) {
        type = std::move(_type);
        name = std::move(_name);
        count = _count;
    }

    RegName(string _type, string _inst) {
        arithOpRex = R"((\w+)\.(\d+))";
        smatch opRexRes;

        type = std::move(_type);
        name = std::move(_inst);
        count = 0;

        if (regex_search(name, opRexRes, arithOpRex)) {
            name = opRexRes[1];
            count = stoi(opRexRes[2]);
        }
    }

    string GetString() {
        string _res = type + " " + name;
        _res += count == 0 ? "" : "." + to_string(count);

        return _res;
    }

    string GetName() {
        return name;
    }

    int GetCount() const {
        return count;
    }

    string GetType() {
        return type;
    }

private:
    regex arithOpRex;

    string type;
    string name;
    int count;
};

class ArithOp {
public:
    ArithOp() {
        res = new RegName;
        lReg = new RegName;
        rReg = new RegName;
    }

    void Init(const smatch &instRexRes) {
        op = instRexRes[2];

        res = new RegName(instRexRes[3], instRexRes[1]);
        lReg = new RegName(instRexRes[3], instRexRes[4]);
        rReg = new RegName(instRexRes[3], instRexRes[5]);
    }

    string GetString() {
        string _res = res->GetString() + " = " + op + " " + res->GetType() + " " + lReg->GetString() + ", " +
                      rReg->GetString();
        return _res;
    }

private:
    string op;

    RegName *res;
    RegName *lReg;
    RegName *rReg;
};

class FuncCall {
public:
    FuncCall() {
        funcArgsRex = R"((\w+)[\s%\\"]*(\w+[\.\d]*)[%\\"]*[, ]*)";
    }

    void Init(const smatch &instRexRes) {
        funcType = instRexRes[3].str();
        funcName = instRexRes[4].str();

        string tmpArgs = instRexRes[5];
        while (!tmpArgs.empty()) {
            smatch argsRexRes;
            if (regex_search(tmpArgs, argsRexRes, funcArgsRex))
                funcArgs.emplace_back(argsRexRes[1], argsRexRes[2]);

            int pos = tmpArgs.find(argsRexRes[0]);
            tmpArgs.erase(pos, argsRexRes[0].length());
        }
    }

    string GetString() {
        string _res;
        _res = "call " + funcType + " @" + funcName + "(";
        for (int i = 0; i < funcArgs.size(); ++i) {
            _res += funcArgs[i].GetString();
            if (i != funcArgs.size() - 1)
                _res += ", ";
        }
        _res += ")";
        return _res;
    }

private:
    regex funcArgsRex;

    string funcType;
    string funcName;
    vector<RegName> funcArgs;
};

class StoreInst {
public:
    void Init(const smatch &instRexRes) {
        ;
    }

    string GetString() {
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
        funcCallRex = R"(\%\"(FunctionCall)\" = (call) (i256) @\"(\w+)\"\((.*)\))";
        storeRex = R"(store (\w+) %\"(\w+)\", (\w+\**) @\"(\w+)\")";
    }

    void InitInst(const string &_inst) {
        smatch instRexRes;
        if (regex_search(_inst, instRexRes, arithInstRex)) {
            arithOp->Init(instRexRes);
            instType = 1;
        } else if (regex_search(_inst, instRexRes, funcCallRex)) {
            funcCall->Init(instRexRes);
            instType = 2;
        } else if (regex_search(_inst, instRexRes, storeRex)) {
            storeInst->Init(instRexRes);
            instType = 3;
        }
    }

    string GetString() const {
        switch (instType) {
            case 1:
                return arithOp->GetString();
            case 2:
                return funcCall->GetString();
            case 3:
                return storeInst->GetString();
            default:
                return "No such inst type!";
        }
    }

    int GetType() const {
        /*
        switch (instType) {
            case 1:
                cout << "Arithmetic operation ";
                break;
            case 2:
                cout << "Function call ";
                break;
            case 3:
                cout << "Store operation ";
                break;
            default:
                cout << "Unknown type ";
                break;
        }
        */
        return instType;
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
    FuncChain() : length(0) {}

    unsigned int GetChainLength() const {
        return length;
    }

    void InitFunc(int _len) {
        length = _len;
        function.resize(_len);
    }

    Function GetFunction(int _index) {
        return function[_index];
    }

private:
    unsigned int length;

    vector<Function> function;
};

class FuncChains {
public:
    FuncChains() : length(0) {}

    unsigned int GetLength() const { return length; }

    void InitChains(uint _len) {
        length = _len;
        funcChain.resize(_len);
    }

    FuncChain GetChain(int _index) {
        return funcChain[_index];
    }

private:
    unsigned int length;

    vector<FuncChain> funcChain;
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
