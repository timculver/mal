#include "reader.hpp"

#include <regex>
#include <vector>
#include <memory>
#include <iostream>

#include "types.hpp"

using namespace std;

MalType* read_form(Reader& reader);

bool Reader::done() const {
  return index >= tokens.size();
}

const string& Reader::peek() const {
    if (done())
      throw Error{"Parse error"};
    return tokens[index];
}

string Reader::next() {
  return move(tokens[index++]);
}


vector<string> tokenizer(string s) {
    static regex re(R"/([\s,]*(~@|[\[\]{}()'`~^@]|"(?:\\.|[^\\"])*"|;.*|[^\s\[\]{}('"`,;)]*))/");
                                                                               
    vector<string> tokens;
    smatch match;
    while (!s.empty() && regex_search(s, match, re)) {
      if (match.empty())
        break;
      string token = match[1];
      if (token.empty())
        break;
      //cout << "token: [" << token << "]\n";
      if (token[0] != ';') {
        tokens.push_back(token);
      }
      //cout << "suffix: [" << match.suffix() << "]\n";
      s = match.suffix();
    }
    return tokens;
}

MalType* read_atom(Reader& reader) {
  static regex string_regex("\"(.*)\"");
  smatch match;
  if (regex_match(reader.peek(), match, string_regex)) {
    string s = match[1];
    reader.next();
    return new MalString(MalString::unescape(s));
  }
  static regex int_regex("-?[0-9]+");
  if (regex_match(reader.peek(), match, int_regex)) {
    int number = 0;
    number = stoi(reader.next());
    return new MalInt(number);
  } else if (reader.peek() == "nil") {
    reader.next();
    return nil;
  } else if (reader.peek() == "true") {
    reader.next();
    return mtrue;
  } else if (reader.peek() == "false") {
    reader.next();
    return mfalse;
  } else if (reader.peek() == "'") {
    reader.next();
    return new MalList(symbol("quote"), new MalList(read_form(reader), eol));
  } else if (reader.peek() == "`") {
    reader.next();
    return new MalList(symbol("quasiquote"), new MalList(read_form(reader), eol));
  } else if (reader.peek() == "~") {
    reader.next();
    return new MalList(symbol("unquote"), new MalList(read_form(reader), eol));
  } else if (reader.peek() == "~@") {
    reader.next();
    return new MalList(symbol("splice-unquote"), new MalList(read_form(reader), eol));
  } else if (reader.peek() == ")") {
    throw Error{"Unmatched `)`"};
  } else {
    return symbol(reader.next());
  }
}

MalType* read_list(Reader& reader);
MalType* read_vector(Reader& reader);
MalType* read_hash(Reader& reader);

MalType* read_form(Reader& reader) {
  if (reader.peek() == "(") {
    return read_list(reader);
  } else if (reader.peek() == "[") {
    return read_vector(reader);
  } else if (reader.peek() == "{") {
    return read_hash(reader);
  } else {
    return read_atom(reader);
  }
}
    
MalType* read_list(Reader& reader) {
  reader.next(); // "("
  vector<MalType*> e;
  while (reader.peek()[0] != ')')
    e.push_back(read_form(reader));
  reader.next(); // ")"
  MalList* cons = eol;
  for (auto iter=e.rbegin(); iter!=e.rend(); ++iter)
    cons = new MalList(*iter, cons);
  return cons;
}

MalType* read_vector(Reader& reader) {
  reader.next(); // "["
  vector<MalType*> e;
  while (reader.peek()[0] != ']')
    e.push_back(read_form(reader));
  reader.next(); // "]"
  return new MalVector(move(e));
}

MalType* read_hash(Reader& reader) {
  reader.next(); // "{"
  MalHash* hash = new MalHash();
  while (reader.peek()[0] != '}')
    hash = hash->assoc(read_form(reader), read_form(reader));
  reader.next(); // "}"
  return hash;
}

Reader::Reader(string s)
  : tokens(tokenizer(s)), index(0)
{ }

MalType* read_str(string s) {
  Reader reader(s);
  if (reader.done())
    return nullptr;
  auto form = read_form(reader);
  if (!reader.done())
    throw Error{"Extraneous input: `" + reader.next() + "`"};
  return form;
}















