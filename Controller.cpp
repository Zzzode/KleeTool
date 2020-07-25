//
// Created by zode on 2020/7/16.
//

#include "Controller.h"

void Controller::ParseJson(const string &folderName) {
    //从文件中读取，保证当前文件夹有.json文件
    string jsonPath = path + "/" + folderName + "/" + folderName + ".json";
    ifstream inFile(jsonPath, ios::in);
    if (!inFile.is_open()) {
        cout << "Error opening file\n";
        exit(0);
    }

    // 以二进制形式读取json文件内容
    ostringstream buf;
    char ch;
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
    DIR *dir;
    struct dirent *ptr;

    if ((dir = opendir(path.c_str())) == nullptr) {
        perror("Open dir error...");
        exit(1);
    }

    while ((ptr = readdir(dir)) != nullptr) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 ||
            ptr->d_type == 10) /// current dir OR parent dir OR link file
            continue;
        else if (ptr->d_type == 4 || ptr->d_type == 8) /// dir | file
            folderNames.emplace_back(ptr->d_name);
    }
    closedir(dir);
}

void Controller::Entry() {
    GetFiles();
    for (const auto &folderName : folderNames) {
        if (folderName.find(R"(.py)") != string::npos)
            continue;

        string thisPath = path + "/" + folderName;
        cout << "This path is " << thisPath << endl;

        ParseJson(folderName);

        FunChains();
    }
}

void Controller::FunChains() {
    assert(document.IsArray());

    // document.Size为调用链数量
    funcChains->InitChains(document.Size());
    int chainIndex = 0;
    for (auto &funChain : document.GetArray()) {
        assert(funChain.IsArray());
        FuncChain thisChain = funcChains->GetChain(chainIndex);
        thisChain.InitFunc(funChain.Size());
        chainIndex++;

        // 每一个循环都是一个函数
        int funcIndex = 0;
        for (auto &funPtr : funChain.GetArray()) {
            Function thisFunc = thisChain.GetFunction(funcIndex);
            thisFunc.SetFuncName(funPtr.MemberBegin()->value.GetString());

            assert((funPtr.MemberBegin() + 1)->value.IsArray());

            thisFunc.InitInsts((funPtr.MemberBegin() + 1)->value.Size());
            cout << "funcName = " << thisFunc.GetFuncName() << endl;

            int instNum = 0;
            for (auto &inst : (funPtr.MemberBegin() + 1)->value.GetArray()) {
                assert(inst.IsObject());
                Instruction thisInst = thisFunc.GetInst(instNum);
                thisInst.InitInst((inst.MemberBegin() + 1)->value.GetString());

                cout << thisInst.GetType() << ": " << thisInst.GetString() << endl;
                instNum++;
            }
            cout << endl;
            funcIndex++;
        }
    }
}

void Controller::ThreadControllor() {
    ;
}
