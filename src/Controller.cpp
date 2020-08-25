//
// Created by zode on 2020/7/16.
//

#include "Controller.h"
#include "ModifyLLVM.h"

#include <unistd.h>

bool Controller::ParseJson(const string& folderName) {
  //从文件中读取，保证当前文件夹有.json文件
  string   jsonPath = path + "/" + folderName + "/" + folderName + ".json";
  ifstream inFile(jsonPath, ios::in);
  if (!inFile.is_open()) {
    cout << "No json file\n";
    return false;
  }

  // 以二进制形式读取json文件内容
  ostringstream buf;
  char          ch;
  while (buf && inFile.get(ch))
    buf.put(ch);

  // 文件流指针重置
  inFile.clear();
  inFile.seekg(0, ios::beg);

  // 解析json及inst
  document.Parse(buf.str().c_str());
  inFile.close();

  return true;
}

bool Controller::GetFiles() {
  DIR*           dir;
  struct dirent* ptr;

  if ((dir = opendir(path.c_str())) == nullptr) {
    perror("Open dir error...");
    return false;
  }

  while ((ptr = readdir(dir)) != nullptr) {
    if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 ||
        ptr->d_type == 10)  /// current dir OR parent dir OR link file
      continue;
    else if (ptr->d_type == 4 || ptr->d_type == 8)  /// dir | file
      folderNames.emplace_back(ptr->d_name);
  }
  closedir(dir);

  return true;
}

void Controller::Entry() {
  if(GetFiles()) {
    for (const auto& folderName : folderNames) {
      // 开始计时

      if (folderName.find(R"(.py)") != string::npos ||
          folderName.find(R"(.sh)") != string::npos)
        continue;

      string thisPath = path + "/" + folderName;
      cout << "This path is " << thisPath << endl;

      // 只有没有跑完的数据会接下去执行
      if (access((thisPath + "/time.txt").c_str(), 0) == -1) {
        start_t = clock();
        // 存在json
        if (ParseJson(folderName))
          FunChains(folderName);

        // 结束计时
        end_t = clock();
        // 输出时间
        double   endtime = (double)(end_t - start_t) / CLOCKS_PER_SEC;
        ofstream out(path + "/" + folderName + "/time.txt");
        out << "time = " << endtime << " s" << endl;
      }
    }
  }
}

void Controller::FunChains(const string& folderName) {
  assert(document.IsArray());

  // document.Size为调用链数量
  funcChains->InitChains(document.Size());
  ModifyLLVM modifyLlvm(folderName, path + "/" + folderName, document.Size());
  LLVMFile*  thisLLVMFile = modifyLlvm.GetLLVMFile();
  cout << "file name = " << thisLLVMFile->GetFileName() << endl;

  int chainIndex = 0;
  // 每一个循环都是一个调用链
  for (auto& funChain : document.GetArray()) {
    cout << "/////// Function Chain " << to_string(chainIndex) << " ///////"
         << endl;
    assert(funChain.IsArray());
    FuncChain thisChain = funcChains->GetChain(chainIndex);
    thisChain.InitFunc(funChain.Size());

    cout << "debug: 1" << endl;
    // TODO 可以在这里定位文件中的调用链
    // 一次定位的目的是减少IO耗时 但实际上最好的方法是一步到位
    // LLVMFuncChain thisLLVMChain = thisLLVMFile->GetLLVMChain(chainIndex);
    // thisLLVMChain.Init(funChain.Size());

    // 每一个循环都是一个函数
    int funcIndex = 0;
    for (auto& funPtr : funChain.GetArray()) {
      Function thisFunc = thisChain.GetFunction(funcIndex);
      thisFunc.SetFuncName(funPtr.MemberBegin()->value.GetString());

      assert((funPtr.MemberBegin() + 1)->value.IsArray());

      thisFunc.InitInsts((funPtr.MemberBegin() + 1)->value.Size());
      // cout << "funcName = " << thisFunc.GetFuncName() << endl;

      // 读入函数文件
      string funcName = thisFunc.GetFuncName();  // cout << funcName << endl;
      LLVMFunction thisLLVMFunc = thisLLVMFile->InitFuncLines(funcName);
      // thisLLVMFunc.Show();

      int instNum = 0;
      // 每一次循环都是一条指令
      for (auto& inst : (funPtr.MemberBegin() + 1)->value.GetArray()) {
        assert(inst.IsObject());
        Instruction thisInst = thisFunc.GetInst(instNum);
        thisInst.InitInst((inst.MemberBegin() + 1)->value.GetString());

        // 打印当前指令
        // cout << thisInst.GetType() << ": " << thisInst.GetString() << endl;

        // 当指令为算数操作时用klee_assume
        // TODO 全局变量需要符号化
        if (thisInst.GetType() == 1) {
          auto arithInst = static_cast<ArithOp*>(thisInst.GetInst());

          int opType =
              arithInst->GetOp() == "add" || arithInst->GetOp() == "mul" ? 1 :
                                                                           2;

          thisLLVMFunc.AddAssume(3 - opType, instNum * 3 + 0,
                                 arithInst->GetReg(2), arithInst->GetReg(2),
                                 arithInst->GetString(), false);
          thisLLVMFunc.AddAssume(3 - opType, instNum * 3 + 1,
                                 arithInst->GetReg(3), arithInst->GetReg(2),
                                 arithInst->GetString(), false);
          thisLLVMFunc.AddAssume(opType + 0, instNum * 3 + 2,
                                 arithInst->GetReg(1), arithInst->GetReg(2),
                                 arithInst->GetString(), true);

          thisFunc.SetIsArith(true);
        } else if (thisInst.GetType() == 2) {
          auto callInst = static_cast<FuncCall*>(thisInst.GetInst());

          // TODO 需要将调用的函数的参数符号化 暂不需要
          //
          // TODO 需要将函数本身的参数进行符号化 暂不需要
          //

          thisFunc.SetIsCall(true);
        } else if (thisInst.GetType() == 3) {
          auto storeInst = static_cast<StoreInst*>(thisInst.GetInst());

          // thisLLVMFile->AddGlobalSymbols(storeInst->GetDest());
          // 将store的全局变量符号化
          thisLLVMFunc.WriteNewLines(
              modifyLlvm.ModifyStoreInst(storeInst, thisLLVMFunc));
          // cout << "debug: " << thisLLVMFile->symCount << endl;
          thisFunc.SetIsInit(true);
        }
        thisFunc.ReturnInst(instNum, thisInst);
        instNum++;
      }
      // 替换文件中当前函数
      thisLLVMFile->Replace(thisLLVMFunc.StartLine(), thisLLVMFunc.EndLine(),
                            thisLLVMFunc.GetNewLines());
      thisLLVMFile->ReturnLLFunc(funcName, thisLLVMFunc);

      thisChain.ReturnFunction(funcIndex, thisFunc);
      funcIndex++;
    }
    cout << "debug: 3" << endl;
    // 找到调用链的初始函数
    Function& startFunc = thisChain.ReturnChainStart();
    // 添加全局sym和局部sym
    thisLLVMFile->AddLocalSymDecl(
        thisLLVMFile->InitFuncLines(startFunc.GetFuncName()));
    // thisLLVMFile->WriteGlobalSymDecl();

    // 找到调用链的末端函数
    cout << "debug: 3.1" << endl;
    string             endFuncName = thisChain.GetFunction(0).GetFuncName();
    vector<KleeAssume> assumes =
        thisLLVMFile->GetLLFuncs()[endFuncName].GetAssumes();
    cout << "debug: 3.2" << endl;
    LLVMFunction thisLLVMFunc = thisLLVMFile->InitFuncLines(endFuncName);
    thisLLVMFile->SetTmpLines();
    cout << "debug: 3.3" << endl;
    vector<string> newStr =
        modifyLlvm.AddArithGlobalSyms(thisLLVMFunc, assumes.front().GetInst());

    // thisLLVMFile->WriteGlobalSymDecl();
    // ！每个算数指令运行一次Klee
    cout << "debug: 4" << endl;
    for (int i = 0; i < (assumes.size() + 1) / 3; i++) {
      vector<KleeAssume> tmpAssumes(assumes.begin() + (3 * i),
                                    assumes.begin() + (3 * i + 3));
      thisLLVMFunc.WriteNewLines(
          modifyLlvm.ModifyAssumes(thisLLVMFunc, tmpAssumes, newStr));
      thisLLVMFile->Replace(thisLLVMFunc.StartLine(), thisLLVMFunc.EndLine(),
                            thisLLVMFunc.GetNewLines());
      // 插入全局变量的符号化声明
      thisLLVMFile->WriteGlobalSymDecl();

      thisLLVMFile->CreateFile("tmp.ll");
      // 调用`klee --entry-point=thisFuncName`
      RunKlee(startFunc.GetFuncName(), folderName, tmpAssumes.front().GetInst(),
              to_string(i), to_string(chainIndex));

      // TODO 找到只初始化全局变量不参与调用链的函数 单独调用`klee
      //

      thisLLVMFunc.Refresh();
      thisLLVMFile->RefreshLines();
    }
    // exit(0);
    thisLLVMFile->Refresh();
    chainIndex++;
  }
}
