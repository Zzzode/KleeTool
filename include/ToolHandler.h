//
// Created by zode on 2021/2/3.
//

#ifndef KLEETOOL_TOOLHANDLER_H
#define KLEETOOL_TOOLHANDLER_H

class ToolHandler {
 private:
//  Interpreter* m_interpreter;
//  TreeStreamWriter *m_pathWriter, *m_symPathWriter;
//  std::unique_ptr<llvm::raw_ostream> m_infoFile;
//
//  SmallString<128> m_outputDirectory;
//
//  unsigned m_numTotalTests;  // Number of tests received from the interpreter
//  unsigned m_numGeneratedTests;  // Number of tests successfully generated
//  unsigned m_pathsExplored;      // number of paths explored so far
//
//  // used for writing .ktest files
//  int m_argc;
//  char** m_argv;
//
// public:
//  KleeHandler(int argc, char** argv);
//  ~KleeHandler();
//
//  llvm::raw_ostream& getInfoStream() const {
//    return *m_infoFile;
//  }
//  /// Returns the number of test cases successfully generated so far
//  unsigned getNumTestCases() {
//    return m_numGeneratedTests;
//  }
//  unsigned getNumPathsExplored() {
//    return m_pathsExplored;
//  }
//  void incPathsExplored() {
//    m_pathsExplored++;
//  }
//
//  void setInterpreter(Interpreter* i);
//
//  void processTestCase(const ExecutionState& state,
//                       const char* errorMessage,
//                       const char* errorSuffix);
//
//  std::string getOutputFilename(const std::string& filename);
//  std::unique_ptr<llvm::raw_fd_ostream>
//  openOutputFile(const std::string& filename);
//  std::string getTestFilename(const std::string& suffix, unsigned id);
//  std::unique_ptr<llvm::raw_fd_ostream> openTestFile(const std::string& suffix,
//                                                     unsigned id);
//
//  // load a .path file
//  static void loadPathFile(std::string name, std::vector<bool>& buffer);
//
//  static void getKTestFilesInDir(std::string directoryPath,
//                                 std::vector<std::string>& results);
//
//  static std::string getRunTimeLibraryPath(const char* argv0);
};

#endif  // KLEETOOL_TOOLHANDLER_H
