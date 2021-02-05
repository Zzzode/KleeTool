//
// Created by zode on 2021/2/3.
//

#ifndef KLEETOOL_JSONPARSER_H
#define KLEETOOL_JSONPARSER_H

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <string>

class JsonParser {
 private:
  std::string LibraryDir;

 public:
  JsonParser(const std::string& _LibraryDir) : LibraryDir(_LibraryDir) {}
  ~JsonParser() {}
};

#endif  // KLEETOOL_JSONPARSER_H
