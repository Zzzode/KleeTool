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


class Function {
public:
    Function() = default;

    string GetFuncName() { return funcName; }

    bool SetFuncName(string _name){
        funcName = std::move(_name);
        return true;
    }

private:
    string funcName;

    vector<vector<string>> instructions;
};

class FuncChain {
public:
    FuncChain() = default;

    unsigned int GetChainLength() {
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
