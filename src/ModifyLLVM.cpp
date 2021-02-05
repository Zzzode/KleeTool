//
// Created by xxxx on xxxx/xx/xx.
//
#include "ModifyLLVM.h"

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
      // _llFunction.ShowAssume();
      funcLines.insert(funcLines.begin() + i + 1, _res.begin(), _res.end());
    }
  }

  return funcLines;
}

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
    }
  }

  return funcLines;
}

int LLVMFile::FindSize(const string& _type) {
  int resSize = 0;
  regex typeDef(R"( = type \{ (.*) \})");
  smatch typeDefRes;
  for (auto& fileLine : fileLines) {
    if (fileLine.find("define ") != string::npos) break;
    if (fileLine.find(_type + " = type") != string::npos) {
      if (regex_search(fileLine, typeDefRes, typeDef)) {
        vector<string> argTypes;
        string _types(typeDefRes[1].str());
        regex structRex(R"((%[\"]*[\w\.\:]*[\"]*))");
        regex typeIntDef(R"(i\d+)");
        regex typePtrDef(R"([%\"\w\.\:]*\*)");
        regex typeArray(R"(\[(\d+) x i(\d+)\])");
        smatch structRexRes, intRexRes, ptrRexRes, arrayRexRes;
        string tmp;
        while (!_types.empty()) {
          tmp = _types;
          if (regex_search(_types, ptrRexRes, typePtrDef))
            argTypes.push_back(ptrRexRes[0].str());
          else if (regex_search(_types, structRexRes, structRex))
            argTypes.push_back(structRexRes[1].str());
          else if (regex_search(_types, intRexRes, typeIntDef))
            argTypes.push_back(intRexRes[0].str());
          else if (regex_search(_types, arrayRexRes, typeArray))
            argTypes.push_back(arrayRexRes[0].str());

          int pos = _types.find(argTypes.back());
          if (pos != string::npos) _types.erase(pos, argTypes.back().length());
          if (_types[0] == ',') _types.erase(0, 2);
          if (_types[0] == ' ') _types.erase(0, 1);
          if (tmp == _types) return 0;
        }
        // Read each member
        regex typeIntSize(R"(i(\d+))");
        smatch intSizeRes, typePtrDefRes;
        for (const auto& argType : argTypes) {
          if (regex_match(argType, intSizeRes, typeIntSize))
            resSize += stoi(intSizeRes[1].str()) / 8;
          else if (regex_match(argType, typePtrDefRes, typePtrDef))
            resSize += 4;
          else if (regex_search(_types, arrayRexRes, typeArray))
            resSize += stoi(arrayRexRes[1].str()) * stoi(arrayRexRes[2]) / 8;
          else
            resSize += FindSize(argType);
        }
        break;
      }
    }
  }
  return resSize;
}

int LLVMFile::AddLocalSymDecl(LLVMFunction _llvmFunc) {
  FuncDefine func;
  smatch funcRes;
  vector<string> funcLines = _llvmFunc.GetNewLines();
  string defineStr = funcLines.front();
  // cout << "debug: " << defineStr << endl;
  regex funcRex(
      R"(define[ linkonce_odr]*[ weak]*[ interal]*[ hidden]*[ dso_local]* (.*) [@%\"]*([\w+\$]*)[\"]*\((.*)\))");
  if (regex_search(defineStr, funcRes, funcRex)) func.Init(funcRes);
  int lineCount = 0;
  for (auto arg : func.GetArgs()) {
    string argType = arg.GetType();
    string argName = arg.GetName();
    // If the first parameter for EOS, the contract itself, skipped
    if (argType.find("%") != string::npos && argName.find("0") != string::npos)
      continue;
    // Temporary store instruction
    string tmpStoreStr("store " + arg.GetString() + ", ");
    // A temporary register object
    RegName* _source =
        new RegName(arg.GetType(), arg.GetAttr(), arg.GetThisName(),
                    arg.GetCount(), arg.GetHasQuote());
    // A temporary string used for substitution
    vector<string> tmpLines;
    string tmpLine;
    // If the type itself is a pointer
    if (argType.find("*") != string::npos) {
      // Current global symbol value
      int num = AddLocalSymDecl(_source);
      tmpLines.push_back("  %\"bitcast_" + to_string(num) + "\" = bitcast " +
                         _source->GetString() + " to i8*");
      tmpLine = "  call void @klee_make_symbolic(i8* " +
                ("%\"bitcast_" + to_string(num) + "\"");

      int sourceSize = 0;
      if (_source->GetSize() == 0) {
        // Find the size from the global declaration
        string _type = _source->GetType();
        int pos = _type.find('*');
        if (pos != string::npos) _type.erase(pos, 1);
        sourceSize = FindSize(_type);
      } else
        sourceSize = _source->GetSize() / 8;
      if (sourceSize != 0) {
        tmpLine += ", i64 " + to_string(sourceSize);
        tmpLine += ", i8* getelementptr inbounds ([";
        tmpLine += to_string(_source->GetPureName().size() + 1) + " x i8], [" +
                   to_string(_source->GetPureName().size() + 1) +
                   " x i8]* @klee_str";
        tmpLine += num == 0 ? "" : "." + to_string(num);
        tmpLine += ", i64 0, i64 0))";
        tmpLines.push_back(tmpLine);
      } else {
        RemoveLoacalSymDecl(_source);
        tmpLines.clear();
      }
    } else {  // If it's not a pointer you have to write your own store
      RegName* _dest = new RegName(_source->GetType() + "*", "%",
                                   "myDest_" + to_string(symCount), 0, false);
      // 当前全局符号数值
      int num = AddLocalSymDecl(_source);
      string myAllocInst("  " + _dest->GetName() + " = alloca " +
                         _source->GetType());
      tmpLines.push_back(myAllocInst);
      string myStoreInst("  store " + _source->GetString() + ", " +
                         _dest->GetString());
      tmpLines.push_back(myStoreInst);
      tmpLines.push_back("  %\"bitcast_" + to_string(num) + "\" = bitcast " +
                         _dest->GetString() + " to i8*");
      tmpLine = "  call void @klee_make_symbolic(i8* " +
                ("%\"bitcast_" + to_string(num) + "\"");
      // TODO might also need to determine size here
      tmpLine += ", i64 " + to_string(_dest->GetSize() / 8);
      tmpLine += ", i8* getelementptr inbounds ([";
      tmpLine += to_string(_source->GetPureName().size() + 1) + " x i8], [" +
                 to_string(_source->GetPureName().size() + 1) +
                 " x i8]* @klee_str";
      tmpLine += num == 0 ? "" : "." + to_string(num);
      tmpLine += ", i64 0, i64 0))";
      tmpLines.push_back(tmpLine);
    }
    if (!tmpLines.empty())
      funcLines.insert(funcLines.begin() + 1 + lineCount, tmpLines.begin(),
                       tmpLines.end());
    lineCount += tmpLines.size();
  }
  int argsCount = func.GetArgs().size();
  vector<string> testFunc;
  testFunc.emplace_back("define internal void @klee_test() #0 {");
  // Add the test call
  for (int i = 0; i < 2 * argsCount; i++) {
    if (i < argsCount) {
      if (func.GetArgs()[i].GetType().empty())
        testFunc.push_back("  %" + to_string(i + 1) + " = alloca " +
                           func.GetFunc()->GetType());
      else
        testFunc.push_back("  %" + to_string(i + 1) + " = alloca " +
                           func.GetArgs()[i].GetType());
    } else {
      if (func.GetArgs()[i - argsCount].GetType().empty())
        testFunc.push_back("  %" + to_string(i + 1) + " = load " +
                           func.GetFunc()->GetType() + ", " +
                           func.GetFunc()->GetType() + "* %" +
                           to_string(i + 1 - argsCount));
      else
        testFunc.push_back("  %" + to_string(i + 1) + " = load " +
                           func.GetArgs()[i - argsCount].GetType() + ", " +
                           func.GetArgs()[i - argsCount].GetType() + "* %" +
                           to_string(i + 1 - argsCount));
    }
  }
  //    string defineLine = funcLines.front();
  if (func.GetFunc()->GetType().find("void") == string::npos) {
    testFunc.push_back("  %" + to_string(2 * argsCount + 1) + " = call " +
                       func.GetFunc()->GetString());
    testFunc.back() += "(";
    if (!func.GetArgs().empty() && !func.GetArgs()[0].GetType().empty())
      testFunc.back() +=
          func.GetArgs()[0].GetType() + " %" + to_string(argsCount + 1);
    for (int i = 1; i < argsCount; i++) {
      if (func.GetArgs()[0].GetType().empty()) continue;
      testFunc.back() += ", " + func.GetArgs()[i].GetType() + " %" +
                         to_string(i + argsCount + 1);
    }
  } else {
    testFunc.push_back("  call " + func.GetFunc()->GetString());
    testFunc.back() +=
        "(" + func.GetArgs()[0].GetType() + " %" + to_string(argsCount + 1);
    for (int i = 1; i < argsCount; i++) {
      testFunc.back() += ", " + func.GetArgs()[i].GetType() + " %" +
                         to_string(i + argsCount + 1);
    }
  }
  testFunc.back() += ")";
  testFunc.emplace_back("  ret void");
  testFunc.emplace_back("}");

  funcLines.emplace_back("");
  funcLines.insert(funcLines.end(), testFunc.begin(), testFunc.end());

  _llvmFunc.WriteNewLines(funcLines);
  Replace(_llvmFunc.StartLine(), _llvmFunc.EndLine(), funcLines);
  return func.GetArgs().size();
}

KleeAssume::KleeAssume(int _opType,
                       int _num,
                       RegName* _nameL,
                       RegName* _nameR,
                       string _inst,
                       bool _first) {
  inst = std::move(_inst);
  count = 1;
  string tmpStr;
  string tmpRes;
  string tmpOp;

  tmpRes = "%\"tmpAssume_" + to_string(_num) + ("." + to_string(count)) + "\"";
  if (_first) {
    if (_opType == 1)
      tmpOp = "icmp ult " + _nameL->GetString() + ", " + _nameR->GetName();
    else if (_opType == 2)
      tmpOp = "icmp ugt " + _nameL->GetString() + ", " + _nameR->GetName();
  } else {
    if (_opType == 1) tmpOp = "icmp ult " + _nameL->GetString() + ", 0";
    else if (_opType == 2)
      tmpOp = "icmp ugt " + _nameL->GetString() + ", 0";
  }
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
