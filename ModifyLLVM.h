//
// Created by zode on 2020/6/29.
//
#ifndef KLEE_MODIFYLLVM_H
#define KLEE_MODIFYLLVM_H

#include <cstring>
#include <set>
#include <string>
#include <utility>
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
    KleeAssume() : count(0) {}

    // 1:Add 2:sub
    KleeAssume(int _opType, int _num, RegName *_nameL) {
        count = 1;
        string tmpStr;
        string tmpRes;
        string tmpOp;

//        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
//        tmpOp = "load i256, " + _nameL->GetString();
//        tmpStr = tmpRes;
//        newStr.push_back("  " + tmpRes + " = " + tmpOp);
//        count++;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        if (_opType == 1)
            tmpOp = "icmp sgt " + _nameL->GetString() + ", 0";
        else if (_opType == 2)
            tmpOp = "icmp slt " + _nameL->GetString() + ", 0";
        tmpStr = tmpRes;
        newStr.push_back("  " + tmpRes + " = " + tmpOp);
        count++;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        tmpOp = "zext i1 " + tmpStr + " to i32";
        tmpStr = tmpRes;
        newStr.push_back("  " + tmpRes + " = " + tmpOp);
        count++;

        tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
        tmpOp = "sext i32 " + tmpStr + " to i64";
        tmpStr = tmpRes;
        newStr.push_back("  " + tmpRes + " = " + tmpOp);

        newStr.push_back("  call void @klee_assume(i64 " + tmpStr + ")");
    }

    void Show() {
        for (const string &line : newStr)
            cout << line << endl;
    }

//    vector<string>::iterator GetPos(string _instStr) {
//        vector<string>::iterator iter;
//
//        return iter;
//    }

    vector<string> GetNewStr() {
        return newStr;
    }

private:
    int count;
    vector<string> newStr;
};

//class GlobalSymDecl {
//public:
//    GlobalSymDecl() = default;
//
//    void Init() {
//
//    }
//
//private:
//    string symName;
//    pair<string, int> gDecl;
//};

class LLVMFunction {
public:
    LLVMFunction() : startLine(0), endLine(0) {}

    LLVMFunction(int _startLine, int _endLine, string _funcName, const vector<string> &_funcLines) {
        startLine = _startLine;
        endLine = _endLine;
        funcName = std::move(_funcName);
        funcLines = _funcLines;
        newLines = _funcLines;
    }

    vector<string> GetLines() {
        return funcLines;
    }

    vector<string> GetNewLines() {
        return newLines;
    }

    void WriteNewLines(vector<string> _newLines){
        newLines = std::move(_newLines);
    }

    void AddAssume(int _opType, int _num, RegName *_nameL) {
        kleeAssumes.emplace_back(_opType, _num, _nameL);
    }

    void ClearAssume(){
        kleeAssumes.clear();
    }

    vector<string> GetAssumeStr(){
        vector<string> _res;
        for(auto kleeAssume : kleeAssumes){
            vector<string> tmp = kleeAssume.GetNewStr();
            _res.insert(_res.end(), tmp.begin(), tmp.end());
        }
        return _res;
    }

    int StartLine(){
        return startLine;
    }
    int EndLine(){
        return endLine;
    }

    void ShowAssume(){
        for(auto kleeAssume : kleeAssumes){
            kleeAssume.Show();
        }
    }

    void Show() {
        for (const auto &funcLine : funcLines)
            cout << funcLine << endl;
    }

private:
    vector<KleeAssume> kleeAssumes;

    int startLine;
    int endLine;
    string funcName;
    vector<string> newLines;
    vector<string> funcLines;
};

class LLVMFuncChain {
public:
    LLVMFuncChain() = default;

    void Init(int _size) {
        LLVMFunctions.resize(_size);
    }

private:
    // 一个函数调用链
    vector<LLVMFunction> LLVMFunctions;
};

class LLVMFile {
public:
    LLVMFile() = default;

    explicit LLVMFile(const string &_name, const string &_path, int _size) {
        fileName = _name + ".ll";
        filePath = _path;
        llFile.open(_path + "/" + fileName, ios::in);
        cout << "debug " << _path + "/" + fileName << endl;
        if (!llFile.is_open())
            cout << "Invalid llvm file!" << endl;

        string tmpLine;
        while (getline(llFile, tmpLine)) {
            fileLines.emplace_back(tmpLine);
//            cout << "!!!" << tmpLine << endl;
        }

        llFile.close();

        funcChains.resize(_size);
    }

    LLVMFuncChain GetLLVMChain(int _index) {
        return funcChains[_index];
    }

    string GetFileName() {
        return fileName;
    }

    // 替换原函数
    void Replace(int _start, int _end, vector<string> _newStr){
        vector<string>::iterator iter;
        iter = fileLines.erase(fileLines.begin() + _start, fileLines.begin() + _end + 1);
        fileLines.insert(iter, _newStr.begin(), _newStr.end());
    }

    /**
     * 返回文件中的行数和函数
     * @param _funcName
     * @return
     */
    LLVMFunction InitFuncLines(const string &_funcName) {
        int startLine = 0, endLine = 0;
        vector<string> funcLines;

        bool inThisFunc = false;
        regex funcRex;
        smatch funcRexRes;
        funcRex = "define .* @" + _funcName + R"(\(.*\))";

//        cout << "!!!" << _funcName << endl;

        for (int i = 0; i < fileLines.size(); ++i) {
            string fileLine = fileLines[i];
//            cout << "!!!" << fileLine << endl;
            if (inThisFunc) {
                funcLines.push_back(fileLine);
                if (fileLine.find('}') != string::npos) {
                    endLine = i;
                    inThisFunc = false;
                    break;
                }
            } else {
                if (regex_search(fileLine, funcRexRes, funcRex)) {
                    startLine = i;
//                    cout << "!!!" << endl;
                    funcLines.push_back(fileLine);
                    inThisFunc = true;
                }
            }
        }

        return LLVMFunction(startLine, endLine, _funcName, funcLines);
    }

    vector<string> GetFileLines() {
        return fileLines;
    }

    // 创建新文件
    void CreateFile(const string& _newName){
        ofstream newFile(filePath + "/" + _newName, ios::out);
        for(const auto& fileLine : fileLines)
            newFile << fileLine << endl;
    }

private:
    fstream llFile;
private:
    string fileName;
    string filePath;
    vector<string> fileLines;
//    string fileLine;

    vector<LLVMFuncChain> funcChains;
};

class ModifyLLVM {
public:
    explicit ModifyLLVM(const string &_name, const string &_path, int _count) {
        kleeSymDecl = "declare void @klee_make_symbolic(i8*, i64, i8*)";
        kleeAssumeDecl = "declare void @klee_assume(i64)";

        llvmFile = new LLVMFile(_name, _path, _count);
    }

    void Modify(const string &_instStr, LLVMFunction _llFunction);

    vector<string> ModifyArithInst(ArithOp *inst, int num, LLVMFunction _llFunction);

    LLVMFile *GetLLVMFile() {
        return llvmFile;
    }

private:
    LLVMFile *llvmFile;
//    vector<KleeAssume> kleeAssume;

private:
    string kleeSymDecl;
    string kleeAssumeDecl;
    string globalSymDecl;
};


void configArgv(const string &tmpArgv);

#endif // KLEE_MODIFYLLVM_H
