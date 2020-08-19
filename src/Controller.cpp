//
// Created by zode on 2020/7/16.
//

#include "Controller.h"
#include "ModifyLLVM.h"

void Controller::ParseJson(const string& folderName) {
  //从文件中读取，保证当前文件夹有.json文件
  string   jsonPath = path + "/" + folderName + "/" + folderName + ".json";
  ifstream inFile(jsonPath, ios::in);
  if (!inFile.is_open()) {
    cout << "Error opening file\n";
    exit(0);
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
}

void Controller::GetFiles() {
  DIR*           dir;
  struct dirent* ptr;

  if ((dir = opendir(path.c_str())) == nullptr) {
    perror("Open dir error...");
    exit(1);
  }

  while ((ptr = readdir(dir)) != nullptr) {
    if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 ||
        ptr->d_type == 10)  /// current dir OR parent dir OR link file
      continue;
    else if (ptr->d_type == 4 || ptr->d_type == 8)  /// dir | file
      folderNames.emplace_back(ptr->d_name);
  }
  closedir(dir);
}

void Controller::Entry() {
  GetFiles();
  for (const auto& folderName : folderNames) {
    if (folderName.find(R"(.py)") != string::npos)
      continue;

    string thisPath = path + "/" + folderName;
    cout << "This path is " << thisPath << endl;

    ParseJson(folderName);

    FunChains(folderName);
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

    // TODO 需要在这里定位文件中的调用链
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

      // TODO 读入函数文件
      string funcName = thisFunc.GetFuncName();  // cout << funcName << endl;
      LLVMFunction thisLLVMFunc = thisLLVMFile->InitFuncLines(funcName);
      // thisLLVMFunc.Show();

      // 每一次循环都是一条指令
      int instNum = 0;
      for (auto& inst : (funPtr.MemberBegin() + 1)->value.GetArray()) {
        assert(inst.IsObject());
        Instruction thisInst = thisFunc.GetInst(instNum);
        thisInst.InitInst((inst.MemberBegin() + 1)->value.GetString());

        // 打印当前指令
        // cout << thisInst.GetType() << ": " << thisInst.GetString() << endl;

        // 当指令为算数操作时用klee_assume
        // 只有全局变量需要符号化
        if (thisInst.GetType() == 1) {
          auto arithInst = static_cast<ArithOp*>(thisInst.GetInst());

          thisLLVMFunc.WriteNewLines(
              modifyLlvm.ModifyArithInst(arithInst, instNum * 3, thisLLVMFunc));
          thisLLVMFunc.ClearAssume();

          thisFunc.SetIsArith(true);
        } else if (thisInst.GetType() == 2) {
          auto callInst = static_cast<FuncCall*>(thisInst.GetInst());

          // TODO 需要将调用的函数的参数符号化 暂不需要
          // thisLLVMFunc.WriteNewLines(modifyLlvm.ModifyCallInst(callInst,
          // thisLLVMFunc));
          // TODO 需要将函数本身的参数进行符号化 暂不需要
          //

          thisFunc.SetIsCall(true);
        } else if (thisInst.GetType() == 3) {
          auto storeInst = static_cast<StoreInst*>(thisInst.GetInst());

          // 需要将store的全局变量符号化
          thisLLVMFunc.WriteNewLines(
              modifyLlvm.ModifyStoreInst(storeInst, thisLLVMFunc));
          //                    cout << "debug: " << thisLLVMFile->symCount <<
          //                    endl;
          thisFunc.SetIsInit(true);
        }
        thisFunc.ReturnInst(instNum, thisInst);
        instNum++;
      }
      // 替换文件中当前函数
      thisLLVMFile->Replace(thisLLVMFunc.StartLine(), thisLLVMFunc.EndLine(),
                            thisLLVMFunc.GetNewLines());
      thisChain.ReturnFunction(funcIndex, thisFunc);

      funcIndex++;
    }
    // 符号化参数和调用函数参数
    Function& lastFunc = thisChain.ReturnChainStart();
    // cout << "debug last function: " << lastFunc.GetFuncName() << endl;
    thisLLVMFile->AddLocalSymDecl(
        thisLLVMFile->InitFuncLines(lastFunc.GetFuncName()));

    // 创建新文件
    thisLLVMFile->WriteGlobalSymDecl();
    thisLLVMFile->CreateFile("tmp.ll");
    chainIndex++;

    // 调用`klee --entry-point=thisFuncName`
    RunKlee(lastFunc.GetFuncName(), folderName);
    thisLLVMFile->Refresh();
    exit(0);
    // TODO 找到只初始化全局变量不参与调用链的函数 单独调用`klee
    // --entry-point=thisFuncName`
  }
}
