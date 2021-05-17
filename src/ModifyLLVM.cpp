//
// Created by xxxx on xxxx/xx/xx.
//
#include "ModifyLLVM.h"

#include <iostream>
#include <regex>

vector<string> ModifyLLVM::AddArithGlobalSyms(LLVMFunction& _llFunction,
                                              const string& _inst) {
  // cout << "debug: 4.0" << endl;
  vector<string> funcLines(_llFunction.GetNewLines());
  vector<string> _res;
  // cout << "debug: 4.1" << endl;
  // init this instruction
  Instruction thisInst;
  thisInst.InitInst(_inst);
  auto thisArithInst = static_cast<ArithOp*>(thisInst.GetInst());
  // search this instruction location in this function lines
  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(_inst) != string::npos) {
      // cout << "debug: 4.2" << endl;
      // Initializes a temporary variable
      Instruction lLoadInst, rLoadInst;
      RegName* rightSource = nullptr;
      RegName* leftSource = nullptr;
      // Determine if left and right operands exist
      if (thisArithInst->GetReg(2)->GetAttr() != "constant") {
        int j = 1;
        while (i - j >= 0) {
          if (funcLines[i - j].find(thisArithInst->GetReg(2)->GetName()) !=
              string::npos) {
            string tmpStr(thisArithInst->GetReg(2)->GetName() + " = load");
            string loadStr;
            if (funcLines[i - j].find(tmpStr) != string::npos) {
              loadStr = funcLines[i - j];
            } else if (funcLines[i - j - 1].find(tmpStr) != string::npos) {
              loadStr = funcLines[i - j - 1];
            } else
              break;
            // If initialized from a global structure
            if (loadStr.find("getelementptr") != string::npos) { break; }
            lLoadInst.InitInst(loadStr);
            auto lInst = static_cast<LoadInst*>(lLoadInst.GetInst());
            leftSource = lInst->GetSource();
            break;
          }
          j++;
        }
        if (leftSource != nullptr) {
          if (leftSource->GetAttr() == "@") {
            if (leftSource->GetName() == "@wasm_rt_call_stack_depth") {
              _res.clear();
              _res.emplace_back("@wasm_rt_call_stack_depth");
              return _res;
            }
            // llvmFile->AddGlobalSymbols(leftSource);
            int num = llvmFile->AddGlobalSymDecl(leftSource);

            string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
            newStr += "(" + leftSource->GetString() + " to i8*), i64 ";
            newStr += to_string(leftSource->GetSize() / 8) +
                      ", i8* getelementptr inbounds ([";
            unsigned int _size = leftSource->GetPureName().size() + 1;
            newStr += to_string(_size) + " x i8], [" + to_string(_size) +
                      " x i8]* @klee_str";
            newStr += num == 0 ? "" : "." + to_string(num);
            newStr += ", i64 0, i64 0))";
            _res.push_back(newStr);
          }
        }
      }
      if (thisArithInst->GetReg(3)->GetAttr() != "constant") {
        int j = 1;
        while (i - j >= 0) {
          if (funcLines[i - j].find(thisArithInst->GetReg(3)->GetName()) !=
              string::npos) {
            string tmpStr(thisArithInst->GetReg(3)->GetName() + " = load");
            string loadStr;
            if (funcLines[i - j].find(tmpStr) != string::npos) {
              loadStr = funcLines[i - j];
            } else if (funcLines[i - j - 1].find(tmpStr) != string::npos) {
              loadStr = funcLines[i - j - 1];
            } else
              break;
            // If initialized from a global structure
            if (loadStr.find("getelementptr") != string::npos) { break; }
            rLoadInst.InitInst(loadStr);
            auto rInst = static_cast<LoadInst*>(rLoadInst.GetInst());
            rightSource = rInst->GetSource();
            break;
          }
          j++;
        }
        if (rightSource != nullptr) {
          if (rightSource->GetAttr() == "@") {
            // If the left Source exists
            if (leftSource != nullptr)
              if (rightSource->GetPureName() == leftSource->GetPureName())
                break;
            if (rightSource->GetName() == "@wasm_rt_call_stack_depth") {
              _res.clear();
              _res.emplace_back("@wasm_rt_call_stack_depth");
              return _res;
            }
            // llvmFile->AddGlobalSymbols(rightSource);
            int num = llvmFile->AddGlobalSymDecl(rightSource);

            string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
            newStr += "(" + rightSource->GetString() + " to i8*), i64 ";
            newStr += to_string(rightSource->GetSize() / 8) +
                      ", i8* getelementptr inbounds ([";
            unsigned int _size = rightSource->GetPureName().size() + 1;
            newStr += to_string(_size) + " x i8], [" + to_string(_size) +
                      " x i8]* @klee_str";
            newStr += num == 0 ? "" : "." + to_string(num);
            newStr += ", i64 0, i64 0))";
            _res.push_back(newStr);
          }
        }
      }
      // cout << "debug: 4.3" << endl;
      break;
    }
  }
  return _res;
}

vector<string> ModifyLLVM::ModifyAssumes(LLVMFunction& _llFunction,
                                         vector<KleeAssume> _assumes,
                                         const vector<string>& _newStr) {
  vector<string> funcLines = _llFunction.GetNewLines();
  string inst = _assumes[0].GetInst();
  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(inst) != string::npos) {
      vector<string> _res(_newStr);
      for (auto assume : _assumes) {
        vector<string> tmp = assume.GetNewStr();
        _res.insert(_res.end(), tmp.begin(), tmp.end());
      }
      funcLines.insert(funcLines.begin() + i + 1, _res.begin(), _res.end());
      funcLine = funcLines[i + _res.size() + 1];
      // AddAssert(funcLines, funcLine, i + _res.size() + 1);
      break;
    }
  }

  return funcLines;
}

/**
 * Temporary don't have to use
 * @param _inst
 * @param _llFunction
 * @return
 */
// vector<string> ModifyLLVM::ModifyCallInst(FuncCall*     _inst,
//                                          LLVMFunction& _llFunction) {
//  vector<string> funcLines = _llFunction.GetNewLines();
//  int            argNum    = _inst->GetArgNum();
//  // _llFunction.Show();
//
//  for (int i = 0; i < funcLines.size(); ++i) {
//    string funcLine = funcLines[i];
//    if (funcLine.find(_inst->GetString()) != string::npos) {
//      ;
//    }
//  }
//
//  return vector<string>();
//}

vector<string> ModifyLLVM::ModifyStoreInst(StoreInst* _inst,
                                           LLVMFunction& _llFunction) {
  vector<string> funcLines = _llFunction.GetNewLines();
  // _llFunction.Show();
  for (int i = 0; i < funcLines.size(); ++i) {
    string funcLine = funcLines[i];
    if (funcLine.find(_inst->GetString()) != string::npos) {
      // Add global variable symbolization
      int num = llvmFile->AddGlobalSymDecl(_inst->GetDest());

      string newStr(R"(  call void @klee_make_symbolic(i8* bitcast )");
      newStr += "(" + _inst->GetDest()->GetString() + " to i8*), i64 ";
      newStr += to_string(_inst->GetDest()->GetSize() / 8) +
                ", i8* getelementptr inbounds ([";
      unsigned int _size = _inst->GetDest()->GetPureName().size() + 1;
      newStr += to_string(_size) + " x i8], [" + to_string(_size) +
                " x i8]* @klee_str";
      newStr += num == 0 ? "" : "." + to_string(num);
      newStr += ", i64 0, i64 0))";

      funcLines.insert(funcLines.begin() + i + 1, newStr);
      // cout << "debug: " << newStr << endl;
    }
  }
  // cout << "debug: " << _llFunction.symCount << endl;
  return funcLines;
}

void ModifyLLVM::AddAssert(vector<string>& funcLines, string& dest, int line) {
  /*
   * format like this
   * %5 = load i32, i32* %1, align 4
   * %6 = icmp slt i32 %5, 0
   * br i1 %6, label %7, label %10
   *
   * %8 = load i32, i32* @kleeCount, align 4
   * %9 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x
   * i8]* @.str, i32 0, i32 0), i32 %8)
   * call void @exit(i32 0) #3
   * unreachable
   */
  Instruction _inst;
  _inst.InitInst(dest);
  auto storeInst = static_cast<StoreInst*>(_inst.GetInst());
  RegName* _dest = storeInst->GetDest();
  vector<string> myAsserts;
  string tmp(R"(  %myAssert = add )" + _dest->GetType() + " " +
             _dest->GetName() +", 0");
  myAsserts.emplace_back(tmp);
  myAsserts.emplace_back("  %myAssert1 = icmp slt " + _dest->GetPureType() +
                         " %myAssert, 0");
  myAsserts.emplace_back("  br i1 %myAssert1, label %then, label %else");
  myAsserts.emplace_back("then:");
  myAsserts.emplace_back("  %myAssert2 = load i32, i32* @kleeCount");
  myAsserts.emplace_back(
      "  %myAssert3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
      "([4 x i8], [4 x i8]* @.output, i32 0, i32 0))");
  myAsserts.emplace_back("  call void @exit(i32 0)");
  myAsserts.emplace_back("  unreachable");
  myAsserts.emplace_back("else:");
  funcLines.insert(funcLines.begin() + line + 1, myAsserts.begin(),
                   myAsserts.end());
  //  for (auto& m : myAsserts) cout << m << endl;
  //  exit(0);
}