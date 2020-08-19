//
// Created by zode on 2020/6/29.
//
#ifndef KLEE_MODIFYLLVM_H
#define KLEE_MODIFYLLVM_H

#include <cstring>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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
  KleeAssume(int _opType, int _num, RegName* _nameL, string _inst) {
    inst  = std::move(_inst);
    count = 1;
    string tmpStr;
    string tmpRes;
    string tmpOp;

    tmpRes =
        "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
    if (_opType == 1)
      tmpOp = "icmp ugt " + _nameL->GetString() + ", 0";
    else if (_opType == 2)
      tmpOp = "icmp ult " + _nameL->GetString() + ", 0";
    tmpStr = tmpRes;
    newStr.push_back("  " + tmpRes + " = " + tmpOp);
    count++;

    tmpRes =
        "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
    tmpOp  = "zext i1 " + tmpStr + " to i32";
    tmpStr = tmpRes;
    newStr.push_back("  " + tmpRes + " = " + tmpOp);
    count++;

    tmpRes =
        "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
    tmpOp  = "sext i32 " + tmpStr + " to i64";
    tmpStr = tmpRes;
    newStr.push_back("  " + tmpRes + " = " + tmpOp);

    newStr.push_back("  call void @klee_assume(i64 " + tmpStr + ")");
  }

  void Show() {
    for (const string& line : newStr)
      cout << line << endl;
  }

  vector<string> GetNewStr() {
    return newStr;
  }

  string GetInst() {
    return inst;
  }

private:
  string         inst;
  int            count;
  vector<string> newStr;
};

class SymDecl {
public:
  SymDecl() = default;

  void Init() {}

private:
  pair<int, RegName> syms;
};

class LLVMFunction {
public:
  LLVMFunction() : startLine(0), endLine(0) {}

  LLVMFunction(int                   _startLine,
               int                   _endLine,
               string                _funcName,
               const vector<string>& _funcLines) {
    //        symCount = 0;
    startLine = _startLine;
    endLine   = _endLine;
    funcName  = std::move(_funcName);
    funcLines = _funcLines;
    newLines  = _funcLines;
  }

  vector<string> GetLines() {
    return funcLines;
  }

  vector<string> GetNewLines() {
    return newLines;
  }

  void Refresh(){
    newLines = vector<string>(funcLines);
  }

  void WriteNewLines(vector<string> _newLines) {
    newLines = std::move(_newLines);
  }

  void AddAssume(int _opType, int _num, RegName* _nameL, const string& _inst) {
    kleeAssumes.emplace_back(_opType, _num, _nameL, _inst);
  }

  vector<KleeAssume> GetAssumes() {
    return kleeAssumes;
  }

  void ClearAssume() {
    kleeAssumes.clear();
  }

  vector<string> GetAssumeStr() {
    vector<string> _res;
    for (auto kleeAssume : kleeAssumes) {
      vector<string> tmp = kleeAssume.GetNewStr();
      _res.insert(_res.end(), tmp.begin(), tmp.end());
    }
    return _res;
  }

  string GetName() {
    return funcName;
  }

  int StartLine() {
    return startLine;
  }

  int EndLine() {
    return endLine;
  }

  void ShowAssume() {
    for (auto kleeAssume : kleeAssumes) {
      kleeAssume.Show();
    }
  }

  void Show() {
    for (const auto& funcLine : funcLines)
      cout << funcLine << endl;
  }

  /*  void AddGlobalSymDecl(RegName* _reg) {
      string _res = "@.str";
      _res += symCount == 0 ? "" : "." + to_string(symCount);
      _res += " = private unnamed_addr constant [";
      _res += to_string(_reg->GetPureName().size() + 1);
      _res += " x i8] c\"" + _reg->GetPureName() + "\\00\"";

      cout << "debug: " << _res << endl;
      globalSymDecls.push_back(_res);
      // str = private unnamed_addr constant [2 x i8] c"x\00";
    }*/

public:
  //    int symCount;

private:
  //    vector<string> globalSymDecls;
  vector<SymDecl>    symDecls;
  vector<KleeAssume> kleeAssumes;

  int            startLine;
  int            endLine;
  string         funcName;
  vector<string> newLines;
  vector<string> funcLines;
};

class LLVMFuncChain {
public:
  LLVMFuncChain() = default;

  void Init(int _size) {
    LLVMFunctions.resize(_size);
  }

  /*  LLVMFunction& GetLLVMFunction(const string& _name) {
      LLVMFunction res;
      for (LLVMFunction& func : LLVMFunctions) {
        if (func.GetName() == _name) {
          return func;
        }
      }
      return res;
    }*/

private:
  // 一个函数调用链
  vector<LLVMFunction> LLVMFunctions;
};

class LLVMFile {
public:
  LLVMFile() : symCount(0) {
    kleeSymDecl    = "declare void @klee_make_symbolic(i8*, i64, i8*)";
    kleeAssumeDecl = "declare void @klee_assume(i64)";
  };

  explicit LLVMFile(const string& _name, const string& _path, int _size) {
    kleeSymDecl    = "declare void @klee_make_symbolic(i8*, i64, i8*)";
    kleeAssumeDecl = "declare void @klee_assume(i64)";

    symCount = 0;
    fileName = _name + ".ll";
    filePath = _path;
    llFile.open(_path + "/" + fileName, ios::in);
    // cout << "debug " << _path + "/" + fileName << endl;
    if (!llFile.is_open())
      cout << "Invalid llvm file!" << endl;

    string tmpLine;
    while (getline(llFile, tmpLine)) {
      fileLines.emplace_back(tmpLine);
      //  cout << "!!!" << tmpLine << endl;
    }
    originFileLines.assign(fileLines.begin(), fileLines.end());
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
  void Replace(int _start, int _end, vector<string> _newStr) {
    vector<string>::iterator iter;
    iter = fileLines.erase(fileLines.begin() + _start,
                           fileLines.begin() + _end + 1);
    fileLines.insert(iter, _newStr.begin(), _newStr.end());
  }

  /**
   * 返回文件中的行数和函数
   * @param _funcName
   * @return
   */
  LLVMFunction InitFuncLines(const string& _funcName) {
    int            startLine = 0, endLine = 0;
    vector<string> funcLines;

    bool   inThisFunc = false;
    regex  funcRex;
    smatch funcRexRes;
    funcRex = "define .* @" + _funcName + R"(\(.*\))";

    //        cout << "!!!" << _funcName << endl;

    for (int i = 0; i < fileLines.size(); ++i) {
      string fileLine = fileLines[i];
      //            cout << "!!!" << fileLine << endl;
      if (inThisFunc) {
        funcLines.push_back(fileLine);
        if (fileLine.find('}') != string::npos) {
          endLine    = i;
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

    LLVMFunction _res(startLine, endLine, _funcName, funcLines);
    llvmFunctions.emplace(_funcName, _res);

    return _res;
  }

  unordered_map<string, LLVMFunction> GetLLFuncs() {
    return llvmFunctions;
  }

  void ReturnLLFunc(const string& _funcName, LLVMFunction& _llFunc) {
    llvmFunctions[_funcName] = _llFunc;
  }

  vector<string> GetFileLines() {
    return fileLines;
  }

  // 创建新文件
  void CreateFile(const string& _newName) {
    ofstream newFile(filePath + "/" + _newName, ios::out);
    for (const auto& fileLine : fileLines)
      newFile << fileLine << endl;

    newFile << kleeSymDecl << endl;
    newFile << kleeAssumeDecl << endl;
  }

  void AddGlobalSymDecl(RegName* _reg) {
    string _res = "@.str";
    _res += symCount == 0 ? "" : "." + to_string(symCount);
    _res += " = private unnamed_addr constant [";
    _res += to_string(_reg->GetPureName().size() + 1);
    _res += " x i8] c\"" + _reg->GetPureName() + "\\00\"";
    // cout << "debug: " << _res << endl;
    globalSymDecls.emplace_back(_res, _reg);

    symCount++;
  }

  void AddLocalSymDecl(RegName* _reg) {
    string _res = "@.str";
    _res += symCount == 0 ? "" : "." + to_string(symCount);
    _res += " = private unnamed_addr constant [";
    _res += to_string(_reg->GetPureName().size() + 1);
    _res += " x i8] c\"" + _reg->GetPureName() + "\\00\"";
    // cout << "debug: " << _res << endl;
    localSymDecls.emplace_back(_res, _reg);

    symCount++;
  }

  void AddLocalSymDecl(LLVMFunction _llvmFunc) {
    FuncDefine     func;
    smatch         funcRes;
    vector<string> funcLines = _llvmFunc.GetNewLines();
    string         defineStr = funcLines.front();
    // cout << "debug: " << defineStr << endl;
    regex funcRex(R"(define (.*) @\"(.*)\"\((.*)\))");
    if (regex_search(defineStr, funcRes, funcRex))
      func.Init(funcRes);

    vector<RegName> funcArgs;
    for (auto arg : func.GetArgs()) {
      for (int i = 0; i < funcLines.size(); ++i) {
        if (funcLines[i].find("store " + arg.GetString() + ", ") !=
            string::npos) {
          StoreInst storeInst;
          storeInst.Init(funcLines[i]);
          RegName* _dest = storeInst.GetDest();
          // cout << "debug dest: " << _dest->GetString() << endl;

          vector<string> tmpLines;
          string         tmpLine;
          tmpLines.push_back("  %\"bitcast_" + to_string(symCount) +
                             "\" = bitcast " + _dest->GetString() + " to i8*");
          tmpLine = "  call void @klee_make_symbolic(i8* " +
                    ("%\"bitcast_" + to_string(symCount) + "\"");
          tmpLine += ", i64 " + to_string(_dest->GetSize() / 8);
          tmpLine += ", i8* getelementptr inbounds ([";
          tmpLine += to_string(_dest->GetPureName().size() + 1) + " x i8], [" +
                     to_string(_dest->GetPureName().size() + 1) +
                     " x i8]* @.str";
          tmpLine += symCount == 0 ? "" : "." + to_string(symCount);
          tmpLine += ", i64 0, i64 0))";
          tmpLines.push_back(tmpLine);

          funcLines.insert(funcLines.begin() + i + 1, tmpLines.begin(),
                           tmpLines.end());
          AddLocalSymDecl(_dest);
        }
      }
    }
    _llvmFunc.WriteNewLines(funcLines);
    Replace(_llvmFunc.StartLine(), _llvmFunc.EndLine(), funcLines);
  }

  void WriteGlobalSymDecl() {
    int lineNum = 2;
    for (auto _tmp : globalSymDecls) {
      for (int i = 0; i < fileLines.size(); ++i) {
        if (fileLines[i].find(_tmp.second->GetName() + " = ") != string::npos) {
          fileLines.insert(fileLines.begin() + i + 1, _tmp.first);
          lineNum = i + 1;
        }
      }
    }
    for (const auto& _tmp : localSymDecls) {
      fileLines.insert(fileLines.begin() + (++lineNum), _tmp.first);
    }
  }

  void Refresh() {
    fileLines.assign(originFileLines.begin(), originFileLines.end());
    symCount = 0;
    globalSymDecls.clear();
    localSymDecls.clear();
  }

  void SetTmpLines() {
    tmpFileLines = vector<string>(fileLines);
  }

  void RefreshLines() {
    fileLines = vector<string>(tmpFileLines);
  }

public:
  int symCount;

private:
  fstream llFile;

private:
  unordered_map<string, LLVMFunction> llvmFunctions;

  vector<pair<string, RegName*>> globalSymDecls;
  vector<pair<string, RegName*>> localSymDecls;

  string fileName;
  string filePath;

  vector<string> tmpFileLines;
  vector<string> fileLines;
  vector<string> originFileLines;
  //    string fileLine;

  vector<LLVMFuncChain> funcChains;

  string kleeSymDecl;
  string kleeAssumeDecl;
};

class ModifyLLVM {
public:
  ModifyLLVM(const string& _name, const string& _path, int _count) {
    llvmFile = new LLVMFile(_name, _path, _count);
  }

  //    void Modify(const string &_instStr, LLVMFunction _llFunction);

  vector<string>
  ModifyArithInst(ArithOp* _inst, int _num, LLVMFunction& _llFunction);

  vector<string> ModifyCallInst(FuncCall* _inst, LLVMFunction& _llFunction);

  vector<string> ModifyStoreInst(StoreInst* _inst, LLVMFunction& _llFunction);

  vector<string> ModifyAssumes(LLVMFunction&      _llFunction,
                               vector<KleeAssume> _assumes);

  LLVMFile* GetLLVMFile() {
    return llvmFile;
  }

private:
  LLVMFile* llvmFile;
  //    vector<KleeAssume> kleeAssume;

private:
  string globalSymDecl;
};

void configArgv(const string& tmpArgv);

#endif  // KLEE_MODIFYLLVM_H
