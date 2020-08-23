//
// Created by zode on 2020/6/29.
//
#include "ModifyLLVM.h"
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>

vector<string> ModifyLLVM::AddArithGlobalSyms(LLVMFunction& _llFunction,
                                              const string& _inst) {
  vector<string> funcLines = _llFunction.GetNewLines();
  vector<string> _res;

  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(_inst) != string::npos) {
      // 遍历前两行寻找load的全局变量
      Instruction leftLoadInst, rightLoadInst;
      leftLoadInst.InitInst(funcLines[i - 2]);
      rightLoadInst.InitInst(funcLines[i - 1]);
      auto     leftInst    = static_cast<LoadInst*>(leftLoadInst.GetInst());
      auto     rightInst   = static_cast<LoadInst*>(rightLoadInst.GetInst());
      RegName* leftSource  = leftInst->GetSource();
      RegName* rightSource = rightInst->GetSource();
      if (leftSource->GetAttr() == "@") {
        // llvmFile->AddGlobalSymbols(leftSource);
        int num = llvmFile->AddGlobalSymDecl(leftSource);

        string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
        newStr += "(" + leftSource->GetString() + " to i8*), i64 ";
        newStr += to_string(leftSource->GetSize() / 8) +
                  ", i8* getelementptr inbounds ([";
        unsigned int _size = leftSource->GetPureName().size() + 1;
        newStr +=
            to_string(_size) + " x i8], [" + to_string(_size) + " x i8]* @.str";
        newStr += num == 0 ? "" : "." + to_string(num);
        newStr += ", i64 0, i64 0))";
        _res.push_back(newStr);
      }
      if (rightSource->GetAttr() == "@" &&
          rightSource->GetPureName() != leftSource->GetPureName()) {
        // llvmFile->AddGlobalSymbols(rightSource);
        int num = llvmFile->AddGlobalSymDecl(rightSource);

        string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
        newStr += "(" + rightSource->GetString() + " to i8*), i64 ";
        newStr += to_string(rightSource->GetSize() / 8) +
                  ", i8* getelementptr inbounds ([";
        unsigned int _size = rightSource->GetPureName().size() + 1;
        newStr +=
            to_string(_size) + " x i8], [" + to_string(_size) + " x i8]* @.str";
        newStr += num == 0 ? "" : "." + to_string(num);
        newStr += ", i64 0, i64 0))";
        _res.push_back(newStr);
      }
      break;
    }
  }
  return _res;
}

vector<string> ModifyLLVM::ModifyAssumes(LLVMFunction&      _llFunction,
                                         vector<KleeAssume> _assumes,
                                         vector<string>     _newStr) {
  vector<string> funcLines = _llFunction.GetNewLines();
  string         inst      = _assumes[0].GetInst();
  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(inst) != string::npos) {
      vector<string> _res(_newStr);
      for (auto assume : _assumes) {
        vector<string> tmp = assume.GetNewStr();
        _res.insert(_res.end(), tmp.begin(), tmp.end());
      }
      // _llFunction.ShowAssume();
      funcLines.insert(funcLines.begin() + i + 1, _res.begin(), _res.end());
    }
  }

  return funcLines;
}

/**
 * 暂时不用
 * @param _inst
 * @param _llFunction
 * @return
 */
vector<string> ModifyLLVM::ModifyCallInst(FuncCall*     _inst,
                                          LLVMFunction& _llFunction) {
  vector<string> funcLines = _llFunction.GetNewLines();
  int            argNum    = _inst->GetArgNum();
  // _llFunction.Show();

  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(_inst->GetString()) != string::npos) {
      ;
    }
  }

  return vector<string>();
}

vector<string> ModifyLLVM::ModifyStoreInst(StoreInst*    _inst,
                                           LLVMFunction& _llFunction) {
  vector<string> funcLines = _llFunction.GetNewLines();
  // _llFunction.Show();
  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(_inst->GetString()) != string::npos) {
      // 添加全局变量符号化
      int num = llvmFile->AddGlobalSymDecl(_inst->GetDest());

      string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
      newStr += "(" + _inst->GetDest()->GetString() + " to i8*), i64 ";
      newStr += to_string(_inst->GetDest()->GetSize() / 8) +
                ", i8* getelementptr inbounds ([";
      unsigned int _size = _inst->GetDest()->GetPureName().size() + 1;
      newStr +=
          to_string(_size) + " x i8], [" + to_string(_size) + " x i8]* @.str";
      newStr += num == 0 ? "" : "." + to_string(num);
      newStr += ", i64 0, i64 0))";

      funcLines.insert(funcLines.begin() + i + 1, newStr);
      // cout << "debug: " << newStr << endl;
    }
  }
  // cout << "debug: " << _llFunction.symCount << endl;
  return funcLines;
}
