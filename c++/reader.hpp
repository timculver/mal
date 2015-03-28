#ifndef READER_HPP
#define READER_HPP

#include <regex>
#include <vector>
#include <memory>

#include "types.hpp"

class Reader {
public:
  Reader(std::string s);
  bool done() const;
  const std::string& peek() const;
  std::string next();
private:
  std::vector<std::string> tokens;
  int index;
};

MalType* read_str(std::string s);

#endif