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

namespace ModifyLLVM {

    vector<string> split(const string &str, const string &pattern) {
        vector<string> res;
        if (str.empty())
            return res;
        //在字符串末尾也加入分隔符，方便截取最后一段
        string strs = str + pattern;
        size_t pos = strs.find(pattern);

        while (pos != std::string::npos) {
            string temp = strs.substr(0, pos);
            res.push_back(temp);
            //去掉已分割的字符串,在剩下的字符串中进行分割
            strs = strs.substr(pos + 1, strs.size());
            pos = strs.find(pattern);
        }

        return res;
    }

    void getFiles(const string &newPath, vector<string> &fileNames) {
        DIR *dir;
        struct dirent *ptr;
        char base[1000];

        if ((dir = opendir(newPath.c_str())) == nullptr) {
            perror("Open dir error...");
            exit(1);
        }

        while ((ptr = readdir(dir)) != nullptr) {
            if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0 ||
                ptr->d_type == 10) /// current dir OR parent dir OR link file
                continue;
            else if (ptr->d_type == 4 || ptr->d_type == 8) /// dir | file
                fileNames.emplace_back(ptr->d_name);
        }
        closedir(dir);
    }

    vector<pair<string, set<vector<string>>>> parseJson(const string &thisPath,
                                                        const string &folderName) {
        //从文件中读取，保证当前文件夹有.json文件
        string jsonPath = thisPath + "/" + folderName + ".json";
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

        // 扫描所有json中的函数名字
        string thisLine;
        vector<string> funNames;
        while (getline(inFile, thisLine)) {
            regex pattern(R"( +\"(\w+)\": \[)");
            smatch regResult;

            if (regex_match(thisLine, regResult, pattern)) {
                funNames.push_back((++regResult.begin())->str());
            }
        }

        // 解析json及inst
        vector<pair<string, set<vector<string>>>> instructions;
        rapidjson::Document document;
        document.Parse(buf.str().c_str());
        for (const auto &funName : funNames) {
            const rapidjson::Value &jsonArray = document[funName.c_str()];
            // 处理每个函数中的算数指令
            set<vector<string>> tmpSet;
            pair<string, set<vector<string>>> tmpPair;
            for (auto &inst : jsonArray.GetArray()) {
                regex pattern(R"((%\w+) = (\w+) (\w+) (%\w+), (.+))");
                smatch regResult;
                vector<string> tmp(5);
                string instStr = inst.GetString();
                if (regex_match(instStr, regResult, pattern))
                    for (int i = 0; i < 5; ++i)
                        tmp[i] = regResult[i + 1].str();

                tmpSet.insert(tmp);
            }

            tmpPair = make_pair(funName, tmpSet);
            instructions.push_back(tmpPair);
        }

        return instructions;
    }

    /**
     * Multithread controller
     * @param thisPath folder path
     * @param folderName llvm ir file name
     * @return
     */
    bool threadControl(const string &thisPath, const string &folderName) {
        // file stream
        ifstream inLLFile;
        fstream funNums;

        // parse json file and return funcs
        vector<pair<string, set<vector<string>>>> funcs;
        cout << "Parsing Json" << endl;
        funcs = parseJson(thisPath, folderName);
        cout << "Done Parsing Json!" << endl;

        // open .ll file
        inLLFile.open(thisPath + "/" + folderName + ".ll", ios::in);
        if (!inLLFile.is_open()) {
            cout << "no `.ll` files" << endl;
            exit(0);
        }
        // read llvm ir file
        string thisLine;
        vector<string> fileLines;
        while (getline(inLLFile, thisLine))
            fileLines.push_back(thisLine);
        inLLFile.close();

        // define and create threads
        if (access((thisPath + "/funNums").c_str(), F_OK) == -1) {
            thread tids[funcs.size()];
            for (int i = 0; i < funcs.size(); ++i) {
                string newFolderPath = thisPath + "/" + folderName;
                if (access((newFolderPath + "/" + funcs[i].first + ".ll").c_str(),
                           F_OK) == 0)
                    continue;
                tids[i] = thread(modifyLLVM, newFolderPath, fileLines, funcs[i]);
            }

            // wait for all threads done
            for (int j = 0; j < funcs.size(); ++j) {
                tids[j].join();
            }

            // done
            funNums.open(thisPath + "/funNums", ios::out);
            funNums << funcs.size() << endl;
            funNums.close();
        }

        //  exit(0);
        return true;
    }

    /**
     * Create and Modify llvm ir file
     * @param newPath  folder path
     * @param llName ll ir file name
     * @return success OR fail
     */
    void modifyLLVM(const string &newPath, vector<string> fileLines,
                    const pair<string, set<vector<string>>> &func) {
        // file stream
        ofstream outLLfile;

        // measure all Functions
        // each fun should be measured independent
        vector<string> globalDeclares;
        vector<string> symbolicLines;

        if (!func.first.empty()) {
            // configure llvm lines
            int gCount = 0; // count global declares
            set<string> vars;
            for (auto instructions : func.second) {
                regex varReg_1(" *" + instructions[3] + R"( = load i32, i32\* (@\w+).*)");
                regex varReg_2(" *" + instructions[4] + R"( = load i32, i32\* (@\w+).*)");
                regex resReg(R"( *store i32 )" + instructions[0] +
                             R"(, i32\* ([@%]\w+).*)");
                smatch loadResult;
                string varName_1, varName_2, resName;

                // 这边需要先确定.ll文件中的最初声明位置
                bool inFunBlock = false;
                for (const auto &fileLine : fileLines) {
                    if (inFunBlock && fileLine.find('}') != string::npos) {
                        inFunBlock = false;
                        break;
                    }
                    if (!inFunBlock) {
                        regex funDeclarePattern(".*@" + func.first + R"(.*\{)");
                        smatch regFunDeclareResult;
                        inFunBlock =
                                regex_match(fileLine, regFunDeclareResult, funDeclarePattern);
                        continue;
                    }

                    varName_1 =
                            regex_match(fileLine, loadResult, varReg_1) && varName_1.empty()
                            ? loadResult[1].str()
                            : varName_1;
                    varName_2 =
                            regex_match(fileLine, loadResult, varReg_2) && varName_2.empty()
                            ? loadResult[1].str()
                            : varName_2;
                    resName = regex_match(fileLine, loadResult, resReg) && resName.empty()
                              ? loadResult[1].str()
                              : resName;
                }
                // ! 我注释了算数操作的结果的符号化 但运行时可能需要
                // vars.push_back(resName);
                if (!varName_1.empty())
                    vars.insert(varName_1);
                if (!varName_2.empty())
                    vars.insert(varName_2);
            }
            // ! 非全局变量的命名为数字
            // FIXME 暂时不符号化函数内变量
            if (!vars.empty()) {
                for (auto &var : vars) {
                    if (var.empty())
                        continue;
                    string tmpGol = R"(@.str)";
                    string tmpSym = R"(call void @klee_make_symbolic(i8* bitcast (i32* )" +
                                    var +
                                    R"( to i8*), i64 4, i8* getelementptr inbounds ([)" +
                                    to_string(var.size()) + R"( x i8], [)" +
                                    to_string(var.size()) + R"( x i8]* @.str)";
                    if (gCount == 0) {
                        tmpGol += " = private unnamed_addr constant [" +
                                  to_string(var.size()) + R"( x i8] c")" +
                                  var.substr(1, var.size() - 1) + R"(\00", align 1)";
                        tmpSym += R"(, i64 0, i64 0)))";
                    } else {
                        tmpGol += "." + to_string(gCount) +
                                  " = private unnamed_addr constant [" +
                                  to_string(var.size()) + R"( x i8] c")" +
                                  var.substr(1, var.size() - 1) + R"(\00", align 1)";
                        tmpSym += "." + to_string(gCount) + R"(, i64 0, i64 0)))";
                    }

                    globalDeclares.push_back(tmpGol);
                    symbolicLines.push_back(tmpSym);
                    gCount++;
                }

                globalDeclares.emplace_back(
                        "declare void @klee_make_symbolic(i8*, i64, i8*)");

                // FIXME 局部变量无法使用`bitcast (%struct.str* @global to i8*)`
                bool inGlobalDeclare = false;
                regex funDeclarePattern(R"(define.*@.*\{)");
                regex thisFunPattern(".*@" + func.first + R"(.*\{)");
                smatch regFunDeclareResult;
                smatch regThisFunResult;
                for (int i = 0; i < fileLines.size(); i++) {
                    string fileLine = fileLines[i];
                    if (fileLine.find(R"(target triple = "x86_64-pc-linux-gnu")") !=
                        string::npos ||
                        fileLine.find(R"(target triple = "x86_64-unknown-linux-gnu")") !=
                        string::npos) {
                        inGlobalDeclare = true;
                        continue;
                    }
                    if (inGlobalDeclare &&
                        fileLine.find("; Function Attrs:") != string::npos) {
                        for (int j = 0; j < globalDeclares.size(); j++) {
                            fileLines.insert(fileLines.begin() + i + j - 1, globalDeclares[j]);
                        }
                        i += globalDeclares.size();
                        inGlobalDeclare = false;
                        continue;
                    }

                    if (!inGlobalDeclare &&
                        regex_match(fileLine, regFunDeclareResult, thisFunPattern)) {
                        for (int j = 0; j < symbolicLines.size(); ++j) {
                            fileLines.insert(fileLines.begin() + i + j + 1,
                                             "  " + symbolicLines[j]);
                        }
                        i += symbolicLines.size();
                        break;
                    }
                }
            }
        }

        // output .ll file
        outLLfile.open(newPath + "/" + func.first + ".ll");
        for (auto fileLine : fileLines)
            outLLfile << fileLine << endl;
        outLLfile.close();
        cout << "Modify LLVM OK!" << endl;
    }

    void configArgv(const string &tmpArgv) {
        string path;
        vector<string> filePaths = split(tmpArgv, "/");
        vector<string> cutPath(filePaths.begin(),
                               filePaths.begin() + filePaths.size() - 1);
        for (const auto &tmpPath : cutPath)
            path += tmpPath.empty() ? "" : "/" + tmpPath;

        string newPath = path + "/" + filePaths.back();
        newPath.erase(newPath.end() - 3, newPath.end());

        if (access(newPath.c_str(), 0) == -1)
            mkdir(newPath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);

        cout << "Start Modifying" << endl;
        if (threadControl(path, split(newPath, "/").back()))
            cout << "Modify OK!" << endl;
        else {
            cout << "Klee ERROR: Modify file Err!" << endl;
            exit(0);
        }
    }
} // namespace ModifyLLVM
