//
// Created by zode on 2021/1/17.
//

#include "Help.h"

Help::Help(vector<string> _lines) {
  syms = 0;
  hasDecl = false;
  fileLines = std::move(_lines);
  newLines = fileLines;
}

void Help::CreateFile(const string& _newName, const string& path) {
  ofstream newFile(path + "/" + _newName, ios::out);
  for (const auto& fileLine : newLines) newFile << fileLine << endl;
}

void Help::Refresh() {
  syms = 0;
  newLines = fileLines;
  hasDecl = false;
}

void Help::AddMyTest(string& _inst, string& _endName, int num) {
  int i = 0;
  regex pattern(R"(define (\w+) (.*))");
  smatch res;
  while (i < newLines.size()) {
    if (regex_search(newLines[i], res, pattern)) {
      if (res[0].str().find(_endName) != string::npos) break;
    }
    i++;
  }
  string newLine = "%\"funcall\" = call " + res[1].str() + " " + res[2].str();
  //    cout << newLine << endl;
  int j = 0;
  stack<int> st;
  while (j < newLine.length() && newLine[j] != '(') j++;
  while (j < newLine.length()) {
    if (newLine[j] == '%') {
      newLine.erase(j, 1);
    } else if (newLine[j] == '"') {
      if (st.empty()) st.emplace(j++);
      else {
        int l = st.top();
        st.pop();
        newLine.erase(l, j - l + 1);
        newLine.insert(l, "1");
        j = l;
      }
    } else
      j++;
  }

  vector<string> lines;
  lines.emplace_back("define i1 @\"mytest\"() {");
  lines.emplace_back("entry:");
  lines.emplace_back("  " + newLine);
  lines.emplace_back("  ret i1 0");
  lines.emplace_back("}");

  newLines.insert(newLines.begin() + newLines.size(), lines.begin(),
                  lines.end());
}

pair<int, int> Help::findFunc(string& _funcName) {
  int stLine = 0, endLine = 0;
  regex pattern(R"(define (\w+) @)" + _funcName + R"(\(\))");
  smatch res;

  while (stLine < newLines.size() &&
         !regex_search(newLines[stLine], res, pattern)) {
    stLine++, endLine++;
  }

  while (endLine < newLines.size() && newLines[endLine] != "}") { endLine++; }

  return {stLine, endLine};
}

void Help::RunKlee(const string& _funcName,
                   const string& chain,
                   const string& inst,
                   const string& _path,
                   const string& _inst) {
  // Create and execute instructions
  if (access(chain.c_str(), 0) == -1) system(("mkdir " + chain).c_str());
  if (access(inst.c_str(), 0) != -1) system(("rm -r " + inst).c_str());
  string command("klee --max-time=3min --entry-point=mytest --output-dir=" +
                 inst + " " + _path + "/tmp.ll");
  cout << command << endl;
  system(command.c_str());
  //
  vector<string> symName{"call.value", "Func_selfdestruct", "Func_call",
                         "Func_delegatecall"};
  ofstream out(inst + "/type.txt");
  for (int i = 0; i < 4; i++) {
    if (_inst.find(symName[i]) != string::npos) out << symName[i];
  }
  out.close();
}
