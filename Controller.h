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
    RegName() : count(0) {}

    RegName(string _type, string _attr, string _name, int _count) {
        type = std::move(_type);
        attr = std::move(_attr);
        name = std::move(_name);
        count = _count;
    }

    /**
     * @sample i160, %, FunctionCall.1
     */
    RegName(string _type, string _attr, string _inst) {
        regRex = R"((\w+)\.(\d+))";
        smatch opRexRes;

        type = std::move(_type);
        attr = std::move(_attr);
        name = std::move(_inst);
        count = 0;

        if (regex_search(name, opRexRes, regRex)) {
            name = opRexRes[1];
            count = stoi(opRexRes[2]);
        }
    }

    /**
     * @sample i160, %\"FunctionCall\"
     */
    RegName(string _type, const string &_str) {
        smatch regRexRes;
        regRex = R"((@|%)[\\"]*(\w+)\.(\d+)[\\"]*)";
        regex regRex1(R"((@|%)[\\"]*(\w+)[\\"]*)");

        type = std::move(_type);
        if (regex_search(_str, regRexRes, regRex)) {
            attr = std::move(regRexRes[1].str());
            name = std::move(regRexRes[2].str());
            count = stoi(regRexRes[3]);
        } else if (regex_search(_str, regRexRes, regRex1)) {
            attr = std::move(regRexRes[1].str());
            name = std::move(regRexRes[2].str());
            count = 0;
        }
    }

    string GetString() {
        string _res = type + " " + GetName();

        return _res;
    }

    string GetName() {
        if (attr.find("constant") != string::npos) {
            return name;
        }
        string _res = attr + "\"" + name;
        _res += count == 0 ? "" : "." + to_string(count);
        _res += "\"";
        return _res;
    }

    int GetCount() const {
        return count;
    }

    string GetType() {
        return type;
    }

private:
    regex regRex;

    string type;
    string name;

    string attr;

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
        op = instRexRes[3];

        res = new RegName(instRexRes[4], instRexRes[1], instRexRes[2]);
        lReg = new RegName(instRexRes[4], instRexRes[5], instRexRes[6]);
        rReg = new RegName(instRexRes[4], instRexRes[7], instRexRes[8]);
    }

    string GetString() {
        string _res = res->GetName() + " = " + op + " " + res->GetType() + " " + lReg->GetName() + ", " +
                      rReg->GetName();
        return _res;
    }

    string GetOp(){
        return op;
    }

    RegName *GetReg(int _num){
        switch (_num) {
            case 1: return res;
            case 2: return lReg;
            case 3: return rReg;
            default:
                break;
        }
        return nullptr;
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
        callRes = new RegName;
        funcArgsRex = R"((\w+) [%@\\"]*(\w+[\.\d]*)[\\"]*[, ]*)";
        funcArgsConstRex = R"((\w+) (\w+)[, ]*)";
        funcArgsRegRex = R"((\w+) (@|%)[\\"](.*)[\\"][, ]*)";
    }

    void Init(const smatch &instRexRes) {
        string _type = instRexRes[3].str();
        callFunc = new RegName(_type, "@", instRexRes[4].str());
        callRes = new RegName(_type, instRexRes[1].str());

        string tmpArgs = instRexRes[5];
        smatch argsRexRes;
        while (regex_search(tmpArgs, argsRexRes, funcArgsRex)) {
            smatch tmpRes;
            string tmpRexStr = argsRexRes[0];
            if (regex_search(tmpRexStr, tmpRes, funcArgsConstRex)) {
                funcArgs.emplace_back(tmpRes[1], "constant", tmpRes[2]);
            } else if (regex_search(tmpRexStr, tmpRes, funcArgsRegRex)) {
                funcArgs.emplace_back(tmpRes[1], tmpRes[2], tmpRes[3]);
            }
            int pos = tmpArgs.find(tmpRexStr);
            tmpArgs.erase(pos, tmpRexStr.length());
        }
    }

    string GetString() {
        string _res = callRes->GetName() + " = ";
        _res += "call " + callFunc->GetString() + "(";
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
    regex funcArgsConstRex;
    regex funcArgsRegRex;

    RegName *callRes;
    RegName *callFunc;
//    string funcType;
    vector<RegName> funcArgs;
};

class StoreInst {
public:
    StoreInst() {
        dest = new RegName;
        source = new RegName;
    }

    void Init(const smatch &instRexRes) {
        source = new RegName(instRexRes[1], instRexRes[2], instRexRes[3]);
        dest = new RegName(instRexRes[4], instRexRes[5], instRexRes[6]);
    }

    string GetString() {
        string _res = "store " + source->GetString() + ", " + dest->GetString();
        return _res;
    }

private:
    RegName *dest;
    RegName *source;
};

class Instruction {
public:
    Instruction() {
        instType = 0;
        arithOp = new ArithOp;
        funcCall = new FuncCall;
        storeInst = new StoreInst;

        arithInstRex = R"((%|@)[\\"](.*)[\\"] = (\w+) (\w+) (%|@)[\\"](.*)[\\"], (%|@)[\\"](.*)[\\"])";
        funcCallRex = R"((.*) = (call) (.*) @\"(.*)\"\((.*)\))";
        storeRex = R"(store (\w+) (%|@)[\\"](.*)[\\"], (\w+\**) (%|@)[\\"](.*)[\\"])";
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

    void *GetInst() {
        switch (instType) {
            case 1:
                return arithOp;
            case 2:
                return funcCall;
            case 3:
                return storeInst;
        }
        return nullptr;
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
    // 指令数量
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
        functions.resize(_len);
    }

    Function GetFunction(int _index) {
        return functions[_index];
    }

    vector<string> GetFuncNames(){
        vector<string> _res;
        for (auto a : functions) {
            _res.push_back(a.GetFuncName());
        }
        return _res;
    }

private:
    unsigned int length;

    vector<Function> functions;
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

    void FunChains(const string &folderName);

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
