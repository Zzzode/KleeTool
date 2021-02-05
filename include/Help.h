//
// Created by zode on 2021/1/17.
//

#ifndef KLEETOOL_HELP_H
#define KLEETOOL_HELP_H

#include "Headers.h"

using namespace std;

class Help {
 public:
  explicit Help(vector<string> _lines);

  void CreateFile(const string& _newName, const string& path);

  void Refresh();

  void AddMyTest(string& _inst, string& _endName, int num);

  [[maybe_unused]] pair<int, int> findFunc(string& _funcName);

  void RunKlee(const string& _funcName,
               const string& chain,
               const string& inst,
               const string& _path,
               const string& _inst);

 private:
  bool hasDecl;
  int syms;
  //  vector<string> global;
  vector<string> fileLines;
  vector<string> newLines;
};

#endif  // KLEETOOL_HELP_H
