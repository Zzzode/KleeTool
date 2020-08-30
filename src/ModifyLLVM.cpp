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
  cout << "debug: 4.0" << endl;
  vector<string> funcLines(_llFunction.GetNewLines());
  vector<string> _res;
  cout << "debug: 4.1" << endl;
  // init this instruction
  Instruction thisInst;
  thisInst.InitInst(_inst);
  auto thisArithInst = static_cast<ArithOp*>(thisInst.GetInst());
  // search this instruction location in this function lines
  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(_inst) != string::npos) {
      cout << "debug: 4.2" << endl;
      // 初始化临时变量
      Instruction lLoadInst, rLoadInst;
      RegName*    rightSource = nullptr;
      RegName*    leftSource  = nullptr;
      // 判断左右操作数是否存在
      if (thisArithInst->GetReg(2)->GetAttr() != "constant") {
        for (int j = 1; j <= 4; j++) {
          if (funcLines[i - j].find(thisArithInst->GetReg(2)->GetName()) !=
              string::npos) {
            if (funcLines[i - j].find(thisArithInst->GetReg(2)->GetName() +
                                      " = load") != string::npos)
              lLoadInst.InitInst(funcLines[i - j]);
            else if (funcLines[i - j - 1].find(
                         thisArithInst->GetReg(2)->GetName() + " = load") !=
                     string::npos)
              lLoadInst.InitInst(funcLines[i - j - 1]);
            else
              break;

            auto lInst = static_cast<LoadInst*>(lLoadInst.GetInst());
            leftSource = lInst->GetSource();
            break;
          }
        }
        if (leftSource != nullptr) {
          if (leftSource->GetAttr() == "@") {
            // llvmFile->AddGlobalSymbols(leftSource);
            int num = llvmFile->AddGlobalSymDecl(leftSource);

            string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
            newStr += "(" + leftSource->GetString() + " to i8*), i64 ";
            newStr += to_string(leftSource->GetSize() / 8) +
                      ", i8* getelementptr inbounds ([";
            unsigned int _size = leftSource->GetPureName().size() + 1;
            newStr += to_string(_size) + " x i8], [" + to_string(_size) +
                      " x i8]* @.str";
            newStr += num == 0 ? "" : "." + to_string(num);
            newStr += ", i64 0, i64 0))";
            _res.push_back(newStr);
          }
        }
      }
      if (thisArithInst->GetReg(3)->GetAttr() != "constant") {
        for (int j = 1; j <= 4; j++) {
          if (funcLines[i - j].find(thisArithInst->GetReg(3)->GetName()) !=
              string::npos) {
            if (funcLines[i - j].find(thisArithInst->GetReg(3)->GetName() +
                                      " = load") != string::npos)
              rLoadInst.InitInst(funcLines[i - j]);
            else if (funcLines[i - j - 1].find(
                         thisArithInst->GetReg(3)->GetName() + " = load") !=
                     string::npos)
              rLoadInst.InitInst(funcLines[i - j - 1]);
            else
              break;

            auto rInst  = static_cast<LoadInst*>(rLoadInst.GetInst());
            rightSource = rInst->GetSource();
            break;
          }
        }
        if (rightSource != nullptr) {
          if (rightSource->GetAttr() == "@") {
            // 如果left source存在
            if (leftSource != nullptr)
              if(rightSource->GetPureName() == leftSource->GetPureName())
                break;
            // llvmFile->AddGlobalSymbols(rightSource);
            int num = llvmFile->AddGlobalSymDecl(rightSource);

            string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
            newStr += "(" + rightSource->GetString() + " to i8*), i64 ";
            newStr += to_string(rightSource->GetSize() / 8) +
                      ", i8* getelementptr inbounds ([";
            unsigned int _size = rightSource->GetPureName().size() + 1;
            newStr += to_string(_size) + " x i8], [" + to_string(_size) +
                      " x i8]* @.str";
            newStr += num == 0 ? "" : "." + to_string(num);
            newStr += ", i64 0, i64 0))";
            _res.push_back(newStr);
          }
        }
      }
      cout << "debug: 4.3" << endl;
      break;
    }
  }
  return _res;
}

vector<string> ModifyLLVM::ModifyAssumes(LLVMFunction&         _llFunction,
                                         vector<KleeAssume>    _assumes,
                                         const vector<string>& _newStr) {
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
