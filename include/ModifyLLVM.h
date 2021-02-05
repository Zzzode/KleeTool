//
// Created by xxxx on xxxx/xx/xx.
//
#ifndef KLEE_MODIFYLLVM_H
#define KLEE_MODIFYLLVM_H

#include "Controller.h"

using namespace std;

/**
 * @brief Only the arithmetic instruction needs klee_assume()
 */
class KleeAssume {
 public:
  KleeAssume() : count(0) {}

  // 1:Add 2:sub
  KleeAssume(int _opType,
             int _num,
             RegName* _nameL,
             RegName* _nameR,
             string _inst,
             bool _first);

  void Show() {
    for (const string& line : newStr) cout << line << endl;
  }

  vector<string> GetNewStr() {
    return newStr;
  }

  string GetInst() {
    return inst;
  }

 private:
  string inst;
  int count;
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

  LLVMFunction(int _startLine,
               int _endLine,
               string _funcName,
               const vector<string>& _funcLines) {
    //        symCount = 0;
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

  void Refresh() {
    newLines = vector<string>(funcLines);
  }

  void WriteNewLines(vector<string> _newLines) {
    newLines = std::move(_newLines);
  }

  void AddSymbolic(string& _name) {
    ;
  }

  void AddAssume(int _opType,
                 int _num,
                 RegName* _nameL,
                 RegName* _nameR,
                 const string& _inst,
                 bool _first) {
    kleeAssumes.emplace_back(_opType, _num, _nameL, _nameR, _inst, _first);
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
    for (auto kleeAssume : kleeAssumes) { kleeAssume.Show(); }
  }

  void Show() {
    for (const auto& funcLine : funcLines) cout << funcLine << endl;
  }

 public:
  //    int symCount;

 private:
  //    vector<string> globalSymDecls;
  vector<SymDecl> symDecls;
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
  LLVMFile() : symCount(0) {
    kleeSymDecl = "declare void @klee_make_symbolic(i8*, i64, i8*)";
    kleeAssumeDecl = "declare void @klee_assume(i64)";
  };

  explicit LLVMFile(const string& _name, const string& _path, int _size) {
    kleeSymDecl = "declare void @klee_make_symbolic(i8*, i64, i8*)";
    kleeAssumeDecl = "declare void @klee_assume(i64)";

    symCount = 0;
    fileName = _name;
    filePath = _path;
    llFile.open(_path + "/" + fileName, ios::in);
    // cout << "debug " << _path + "/" + fileName << endl;
    if (!llFile.is_open()) cout << "Invalid llvm file!" << endl;

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

  // Substitution function
  void Replace(int _start, int _end, vector<string> _newStr) {
    vector<string>::iterator iter;
    iter = fileLines.erase(fileLines.begin() + _start,
                           fileLines.begin() + _end + 1);
    fileLines.insert(iter, _newStr.begin(), _newStr.end());
  }

  /**
   * Returns the number of lines and functions in the file
   * @param _funcName
   * @return
   */
  LLVMFunction InitFuncLines(const string& _funcName) {
    int startLine = 0, endLine = 0;
    vector<string> funcLines;

    bool inThisFunc = false;
    regex funcRex;
    smatch funcRexRes;
    string tmpStr("define .* @" + _funcName + R"(\(.*\))");
    for (int i = 0; i < tmpStr.length(); i++) {
      if (tmpStr[i] == '$') {
        tmpStr[i] = '\\';
        tmpStr.insert(tmpStr.begin() + i + 1, '$');
        i++;
      }
    }
    //    cout << tmpStr << endl;
    funcRex = tmpStr;

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
          //  cout << "!!!" << endl;
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

  // Create a new file
  void CreateFile(const string& _newName) {
    ofstream newFile(filePath + "/" + _newName, ios::out);
    for (const auto& fileLine : fileLines) newFile << fileLine << endl;

    newFile << kleeSymDecl << endl;
    newFile << kleeAssumeDecl << endl;
  }

  int AddGlobalSymDecl(RegName* _reg) {
    if (globalSyms.find(_reg->GetPureName()) == localSyms.end()) {
      string _res = "@klee_str";
      _res += symCount == 0 ? "" : "." + to_string(symCount);
      _res += " = private unnamed_addr constant [";
      _res += to_string(_reg->GetPureName().size() + 1);
      _res += " x i8] c\"" + _reg->GetPureName() + "\\00\"";
      // cout << "debug: " << _res << endl;
      globalSymDecls.emplace_back(_res, _reg);
      globalSyms.emplace(_reg->GetPureName(), Symbol(symCount, _res));
      symCount++;
    }

    return globalSyms[_reg->GetPureName()].GetNum();
  }

  int AddLocalSymDecl(RegName* _reg) {
    if (localSyms.find(_reg->GetPureName()) == localSyms.end()) {
      string _res = "@klee_str";
      _res += symCount == 0 ? "" : "." + to_string(symCount);
      _res += " = private unnamed_addr constant [";
      _res += to_string(_reg->GetPureName().size() + 1);
      _res += " x i8] c\"" + _reg->GetPureName() + "\\00\"";
      localSymDecls.emplace_back(_res, _reg);
      localSyms.emplace(_reg->GetPureName(), Symbol(symCount, _res));
      symCount++;
    }

    return localSyms[_reg->GetPureName()].GetNum();
  }

  void RemoveLoacalSymDecl(RegName* _reg) {
    localSyms.erase(_reg->GetPureName());
    symCount--;
  }

  inline int FindSize(const string& _type);

  [[maybe_unused]] int AddLocalSymDecl(LLVMFunction _llvmFunc);

  void WriteGlobalSymDecl() {
    int lineNum = 2;
    for (auto _tmp : globalSymDecls) {
      for (int i = 0; i < fileLines.size(); ++i) {
        if (fileLines[i].find(_tmp.second->GetName() + " = ") != string::npos) {
          fileLines.insert(fileLines.begin() + i + 1, _tmp.first);
          lineNum = i + 1;
          break;
        }
      }
    }
    if (lineNum == 2) {
      while (!fileLines[lineNum].empty()) lineNum++;
    }
    for (const auto& _tmp : localSymDecls) {
      fileLines.insert(fileLines.begin() + (++lineNum), _tmp.first);
    }
  }

  void AddGlobalSymbols(RegName* _reg) {
    globalSymbols.emplace(_reg);
    symCount++;
  }

  void Refresh() {
    fileLines.assign(originFileLines.begin(), originFileLines.end());
    symCount = 0;
    globalSymDecls.clear();
    localSymDecls.clear();
    globalSyms.clear();
    localSyms.clear();
  }

  void SetTmpLines() {
    tmpFileLines = vector<string>(fileLines);
  }

  void RefreshLines() {
    fileLines = vector<string>(tmpFileLines);
    globalSymbols.clear();
  }

 public:
  int symCount;

 private:
  fstream llFile;

 private:
  unordered_map<string, LLVMFunction> llvmFunctions;
  // Global variables for store and Load
  set<RegName*> globalSymbols;

  unordered_map<string, Symbol> globalSyms;
  unordered_map<string, Symbol> localSyms;

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

  vector<string> ModifyAssumes(LLVMFunction& _llFunction,
                               vector<KleeAssume> _assumes,
                               const vector<string>& _newStr);

  vector<string> AddArithGlobalSyms(LLVMFunction& _llFunction,
                                    const string& _inst);

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
