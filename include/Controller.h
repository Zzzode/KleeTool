// Created by zode on 2020/7/16.
//

#ifndef MODIFYLLVM_CONTROLLER_H
#define MODIFYLLVM_CONTROLLER_H

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <regex>
#include <set>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

using namespace std;

class RegName {
public:
  RegName() : count(0) {}

  RegName(string _type, string _attr, string _name, int _count) {
    type  = std::move(_type);
    attr  = std::move(_attr);
    name  = std::move(_name);
    count = _count;
  }

  /**
   * @sample i160, %, FunctionCall.1
   */
  RegName(string _type, string _attr, string _inst) {
    regRex = R"((\w+)\.(\d+))";
    smatch opRexRes;

    type  = std::move(_type);
    attr  = std::move(_attr);
    name  = std::move(_inst);
    count = 0;

    if (regex_search(name, opRexRes, regRex)) {
      name  = opRexRes[1];
      count = stoi(opRexRes[2]);
    }
  }

  /**
   * @sample i160, %\"FunctionCall\"
   */
  RegName(string _type, const string& _str) {
    smatch regRexRes;
    regRex = R"((@|%)[\\"]*(\w+)\.(\d+)[\\"]*)";
    regex regRex1(R"((@|%)[\\"]*(\w+)[\\"]*)");

    type = std::move(_type);
    if (regex_search(_str, regRexRes, regRex)) {
      attr  = std::move(regRexRes[1].str());
      name  = std::move(regRexRes[2].str());
      count = stoi(regRexRes[3]);
    } else if (regex_search(_str, regRexRes, regRex1)) {
      attr  = std::move(regRexRes[1].str());
      name  = std::move(regRexRes[2].str());
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

  string GetPureName() {
    if (attr.find("constant") != string::npos) {
      return name;
    }
    string _res = name;
    _res += count == 0 ? "" : "." + to_string(count);
    return _res;
  }

  int GetCount() const {
    return count;
  }
  string GetAttr() {
    return attr;
  }

  string GetType() {
    return type;
  }

  string GetThisName();

  int GetSize() {
    int    size = 0;
    regex  sizeRex(R"(i(\w+).*)");
    smatch res;
    if (regex_search(type, res, sizeRex)) {
      size = stoi(res[1].str());
    }
    return size;
  }

private:
  regex  regRex;
  string type;
  string name;
  string attr;
  int    count;
};

class ArithOp {
public:
  ArithOp() {
    res  = new RegName;
    lReg = new RegName;
    rReg = new RegName;

    varRex      = R"((%|@)[\\"](.*)[\\"])";
    constantRex = R"(\w+)";
  }

  ArithOp(ArithOp* inst) {
    ;
  }

  void Init(const smatch& instRexRes) {
    op = instRexRes[3];

    res = new RegName(instRexRes[4], instRexRes[1], instRexRes[2]);

    smatch leftRes, rightRes;
    string leftOp(instRexRes[5]), rightOp(instRexRes[6]);
    if (regex_search(leftOp, leftRes, varRex))
      lReg = new RegName(instRexRes[4], leftRes[1], leftRes[2]);
    else if (regex_search(leftOp, leftRes, constantRex))
      lReg = new RegName(instRexRes[4], "constant", leftRes[1]);
    if (regex_search(rightOp, rightRes, varRex))
      rReg = new RegName(instRexRes[4], rightRes[1], rightRes[2]);
    else if (regex_search(rightOp, rightRes, constantRex))
      rReg = new RegName(instRexRes[4], "constant", rightRes[1]);
  }

  string GetString() {
    string _res = res->GetName() + " = " + op + " " + res->GetType() + " " +
                  lReg->GetName() + ", " + rReg->GetName();
    return _res;
  }

  string GetOp() {
    return op;
  }

  RegName* GetReg(int _num) {
    switch (_num) {
      case 1: return res;
      case 2: return lReg;
      case 3: return rReg;
      default: break;
    }
    return nullptr;
  }

private:
  regex varRex;
  regex constantRex;

  string op;

  RegName* res;
  RegName* lReg;
  RegName* rReg;
};

class FuncCall {
public:
  FuncCall() {
    callRes          = new RegName;
    callFunc         = new RegName;
    funcArgsRex      = R"((\w+) [%@\\"]*(\w+[\.\d]*)[\\"]*[, ]*)";
    funcArgsConstRex = R"((\w+) (\w+)[, ]*)";
    funcArgsRegRex   = R"((\w+) (@|%)[\\"](.*)[\\"][, ]*)";
  }

  void Init(const smatch& instRexRes) {
    string _type = instRexRes[3].str();
    callFunc     = new RegName(_type, "@", instRexRes[4].str());
    callRes      = new RegName(_type, instRexRes[1].str());

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

  vector<RegName> GetArgs() {
    return funcArgs;
  }

  int GetArgNum() {
    return funcArgs.size();
  }

private:
  regex funcArgsRex;
  regex funcArgsConstRex;
  regex funcArgsRegRex;

  RegName* callRes;
  RegName* callFunc;

  vector<RegName> funcArgs;
};

class FuncDefine {
public:
  FuncDefine() {
    func             = new RegName;
    funcArgsRex      = R"((\w+) [%@\\"]*(\w*[\.\d]*)[\\"]*[, ]*)";
    funcArgsConstRex = R"((\w+) (\w+)[, ]*)";
    funcArgsRegRex   = R"((\w+) (@|%)[\\"](.*)[\\"][, ]*)";
  }

  void Init(const smatch& instRexRes) {
    string _type = instRexRes[1].str();
    func         = new RegName(_type, "@", instRexRes[2].str());

    string tmpArgs = instRexRes[3];
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

  vector<RegName> GetArgs() {
    return funcArgs;
  }

  int GetArgNum() {
    return funcArgs.size();
  }

private:
  regex funcArgsRex;
  regex funcArgsConstRex;
  regex funcArgsRegRex;

  RegName*        func;
  vector<RegName> funcArgs;
};

class StoreInst {
public:
  StoreInst() {
    storeRex =
        R"(store (\w+) (%|@)[\\"](.*)[\\"], (\w+\**) (%|@)[\\"](.*)[\\"])";
    dest   = new RegName;
    source = new RegName;
  }

  void Init(const smatch& instRexRes) {
    source = new RegName(instRexRes[1], instRexRes[2], instRexRes[3]);
    dest   = new RegName(instRexRes[4], instRexRes[5], instRexRes[6]);
  }

  void Init(const string& _inst) {
    smatch instRexRes;
    if (regex_search(_inst, instRexRes, storeRex)) {
      Init(instRexRes);
    }
  }

  string GetString() {
    string _res = "store " + source->GetString() + ", " + dest->GetString();
    return _res;
  }

  RegName* GetDest() {
    return dest;
  }

  RegName* GetSource() {
    return source;
  }

private:
  regex    storeRex;
  RegName* dest;
  RegName* source;
};

class LoadInst {
public:
  explicit LoadInst(const smatch& _instRexRes) {
    source = new RegName(_instRexRes[4], _instRexRes[5], _instRexRes[6]);
    dest   = new RegName(_instRexRes[3], _instRexRes[1], _instRexRes[2]);
  }

  string GetString() {
    string _res(dest->GetName() + " = load " + dest->GetType() + ", " +
                source->GetString());
    return _res;
  }

  RegName* GetSource() {
    return source;
  }

  RegName* GetDest() {
    return dest;
  }

private:
  regex    loadRex;
  RegName* dest;
  RegName* source;
};

class Symbol {
public:
  Symbol() : num(0) {}
  Symbol(string _str) : num(0), str(std::move(_str)) {}
  Symbol(int _num, string _str) : num(_num), str(std::move(_str)) {}

  int GetNum() {
    return num;
  }

  string GetStr() {
    return str;
  }

  RegName GetSymbol() {
    return symbol;
  }

private:
  int     num;
  string  str;
  RegName symbol;
};

class Instruction {
public:
  Instruction() {
    instType  = 0;
    arithOp   = new ArithOp;
    funcCall  = new FuncCall;
    storeInst = new StoreInst;

    arithInstRex = R"((%|@)[\\"](.*)[\\"] = (\w+) (\w+) (.*), (.*))";
    funcCallRex  = R"((.*) = (call) (.*) @\"(.*)\"\((.*)\))";
    storeRex =
        R"(store (\w+) (%|@)[\\"](.*)[\\"], (\w+\**) (%|@)[\\"](.*)[\\"])";
    loadRex =
        R"((%|@)[\\"](.*)[\\"] = load (\w+), (\w+\**) (%|@)[\\"](.*)[\\"])";
  }

  explicit Instruction(const string& _inst) {
    arithInstRex = R"((%|@)[\\"](.*)[\\"] = (\w+) (\w+) (.*), (.*))";
    funcCallRex  = R"((.*) = (call) (.*) @\"(.*)\"\((.*)\))";
    storeRex =
        R"(store (\w+) (%|@)[\\"](.*)[\\"], (\w+\**) (%|@)[\\"](.*)[\\"])";
    loadRex =
        R"((%|@)[\\"](.*)[\\"] = load (\w+), (\w+\**) (%|@)[\\"](.*)[\\"])";
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
    } else if (regex_search(_inst, instRexRes, loadRex)) {
      loadInst = new LoadInst(instRexRes);
      instType = 4;
    }
  }

  void InitInst(const string& _inst) {
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
    } else if (regex_search(_inst, instRexRes, loadRex)) {
      loadInst = new LoadInst(instRexRes);
      instType = 4;
    }
  }

  string GetString() const {
    switch (instType) {
      case 1: return arithOp->GetString();
      case 2: return funcCall->GetString();
      case 3: return storeInst->GetString();
      case 4: return loadInst->GetString();
      default: return "No such inst type!";
    }
  }

  void* GetInst() {
    switch (instType) {
      case 1: return arithOp;
      case 2: return funcCall;
      case 3: return storeInst;
      case 4: return loadInst;
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

  ArithOp*   arithOp{};
  FuncCall*  funcCall{};
  StoreInst* storeInst{};
  LoadInst*  loadInst;

private:
  regex arithInstRex;
  regex funcCallRex;
  regex storeRex;
  regex loadRex;
};

class Function {
public:
  Function() : instLength(0), isCall(false), isInit(false), isArith(false) {}

  string GetFuncName() {
    return funcName;
  }

  bool SetFuncName(string _name) {
    funcName = std::move(_name);
    return true;
  }

  Instruction GetInst(int _index) {
    return instructions[_index];
  }

  void ReturnInst(int _index, Instruction _inst) {
    instructions[_index] = std::move(_inst);
  }

  void InitInsts(unsigned int _len) {
    instructions.resize(_len);
    instLength = _len;
  }

  unsigned int GetInstNum() const {
    return instLength;
  }

  void SetIsCall(bool _bool) {
    isCall = _bool;
  }

  void SetIsInit(bool _bool) {
    isInit = _bool;
  }

  void SetIsArith(bool _bool) {
    isArith = _bool;
  }

  bool IsCall() const {
    return isCall;
  }

  bool IsInit() const {
    return isInit;
  }

  bool IsArith() const {
    return isArith;
  }

private:
  bool isArith;
  bool isCall;
  bool isInit;
  // 指令数量
  unsigned int        instLength;
  string              funcName;
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

  vector<string> GetFuncNames() {
    vector<string> _res;
    for (auto a : functions) {
      _res.push_back(a.GetFuncName());
    }
    return _res;
  }

  void ReturnFunction(int _index, Function _function) {
    functions[_index] = std::move(_function);
  }

  Function& ReturnChainStart() {
    Function& _res = functions.front();
    if (functions.size() == 1)
      return _res;
    for (int i = 0; i < functions.size(); ++i) {
      // cout << "debug: " << functions[i].IsArith() << functions[i].IsCall() <<
      // functions[i].IsInit() << endl;
      if (functions[i].IsInit() && !functions[i].IsCall()) {
        return functions[i - 1];
      }
    }
    return functions.back();
  }

private:
  unsigned int     length;
  vector<Function> functions;
};

class FuncChains {
public:
  FuncChains() : length(0) {}

  unsigned int GetLength() const {
    return length;
  }

  void InitChains(uint _len) {
    length = _len;
    funcChain.resize(_len);
  }

  FuncChain GetChain(int _index) {
    return funcChain[_index];
  }

private:
  unsigned int      length;
  vector<FuncChain> funcChain;
};

class Controller {
public:
  explicit Controller(string _path) : path(std::move(_path)) {
    funcChains  = new FuncChains;
    threadCount = 10;
  }

  void Entry();

  void GetFiles();

  bool ParseJson(const string& folderName);

  void FunChains(const string& folderName);

  void ThreadControllor();

  void RunKlee(const string& _funcName,
               const string& _folderName,
               const string& _inst,
               const string& _instIndex,
               const string& _chainIndex) {
    string _path(path + "/" + _folderName);
    string _outFolder(path + "/" + _folderName + "/chain" + _chainIndex);
    string _outPath(_outFolder + "/inst" + _instIndex);

    chdir(_outFolder.c_str());
    if (access(_outFolder.c_str(), 0) != -1) {
      if (access(_outPath.c_str(), 0) != -1)
        system(("rm -r " + _outPath).c_str());
    } else
      mkdir(_outFolder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    string command("klee --entry-point=" + _funcName +
                   " --output-dir=" + _outPath + " " + _path + "/tmp.ll");

    cout << endl;
    cout << command << endl;
    system(command.c_str());

    ExtractInfo(_funcName, _folderName, _inst, _instIndex, _chainIndex);
  }

  void ExtractInfo(const string& _funcName,
                   const string& _folderName,
                   const string& _inst,
                   const string& _instIndex,
                   const string& _chainIndex) {
    string _path(path + "/" + _folderName);
    string _outFolder(path + "/" + _folderName + "/chain" + _chainIndex);
    string _outPath(_outFolder + "/inst" + _instIndex);

    ofstream out;
    ifstream in;
    // 输出指令
    out.open(_outPath + "/inst.txt");
    // 判断文件是否已经打开
    if (out.is_open())
      out << _inst << endl;
    out.close();
    // 获取所有ktest文件名
    vector<string> ktestFiles;
    if (GetKtestFile(_outPath, ktestFiles)) {
      for (int i = 0; i < ktestFiles.size(); i++) {
        string command("ktest-tool " + _outPath + "/" + ktestFiles[i] + " > " +
                       _outPath + "/res" + to_string(i) + ".txt");
        system(command.c_str());
        // cout << command << endl;
      }
    }
  }

  static bool GetKtestFile(string& _path, vector<string>& _files) {
    DIR*           dir;
    struct dirent* ptr;

    if ((dir = opendir(_path.c_str())) == nullptr) {
      perror("Open dir error...");
      return false;
    }

    while ((ptr = readdir(dir)) != nullptr) {
      if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 ||
          ptr->d_type == 10)
        continue;
      else if (ptr->d_type == 4 || ptr->d_type == 8) {
        string name = ptr->d_name;
        if (name.find(".ktest") != string::npos)
          _files.emplace_back(ptr->d_name);
      }
    }
    closedir(dir);
    return true;
  }

private:
  string              path;
  vector<string>      folderNames;
  string              jsonName;
  rapidjson::Document document;
  FuncChains*         funcChains;

private:
  clock_t start, end;
  mutex   threadMutex;
  int     threadCount;
};

#endif  // MODIFYLLVM_CONTROLLER_H
