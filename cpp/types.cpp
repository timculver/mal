#include "types.hpp"

#include <sstream>
#include <regex>
#include <unordered_map>

using namespace std;

bool MalVector::equal(MalType* other_object) const {
  auto other = dynamic_cast<MalVector*>(other_object);
  if (e.size() != other->e.size())
    return false;
  for (int ii=0; ii<e.size(); ii++)
    if (!::equal(e[ii], other->e[ii]))
      return false;
  return true;
}

string MalVector::print(bool print_readably) const {
  stringstream s;
  s << '[';
  int ii = 0;
  for (const auto& element : e) {
    if (ii++)
      s << ' ';
    s << element->print(print_readably);
  }
  s << ']';
  return s.str();
}

MalEol* eol = new MalEol();
MalNil* nil = new MalNil();
MalTrue* mtrue = new MalTrue();
MalFalse* mfalse = new MalFalse();

string MalList::print(bool print_readably) const {
  stringstream s;
  s << '(';
  const MalList* p = this;
  while (p != eol) {
    if (p != this)
      s << ' ';
    s << p->car->print(print_readably);
    p = p->cdr;
  }
  s << ')';
  return s.str();
}

int MalList::size() {
  int s = 0;
  for (MalList* p=this; p != eol; p=p->cdr)
    s++;
  return s;
}

bool MalList::equal(MalType* other_obj) const {
  MalList* other = static_cast<MalList*>(other_obj);
  // The typeid check has already taken care of all eol cases.
  return ::equal(car, other->car) && ::equal(cdr, other->cdr);
}

string MalHash::print(bool print_readably) const {
  stringstream s;
  s << '{';
  int ii = 0;
  forEach(tree, [&](const KeyValue& pair) {
    if (ii++)
      s << ' ';
    s << pair.key->print(print_readably) << ' ' << pair.value->print(print_readably);
  });
  s << '}';
  return s.str();
}

MalHash* MalHash::assoc(MalType* key, MalType* value) {
  if (!dynamic_cast<MalString*>(key) && !dynamic_cast<MalSymbol*>(key))
    throw Error{"Expected String or Symbol"};
  if (tree.member(KeyValue{key, value}))
    return this;
  return new MalHash(tree.inserted(KeyValue{key, value}));
}

string MalInt::print(bool) const {
  stringstream s;
  s << v;
  return s.str();
}

string MalSymbol::print(bool) const {
  return s;
}

MalSymbol* symbol(const string& s) {
  static unordered_map<string, MalSymbol*> table;
  auto it = table.find(s);
  if (it != table.end())
    return it->second;
  auto sym = new MalSymbol(s);
  table[s] = sym;
  return sym;
}

string MalString::print(bool print_readably) const {
  return print_string(s, print_readably);
}

string MalString::print_string(string s, bool print_readably) {
  if (print_readably)
    return string("\"") + escape(move(s)) + "\"";
  else
    return s;
}

string MalString::unescape(string s) {
  static regex re1("\\\\\""), re2("\\\\\\\\"), re3("\\\\n");
  return regex_replace(regex_replace(regex_replace(s, re1, "\""), re2, "\\"), re3, "\n");
}

string MalString::escape(string s) {
  static regex re1("\""), re2("\\\\"), re3("\\n");
  return regex_replace(regex_replace(regex_replace(s, re2, "\\\\"), re1, "\\\""), re3, "\\n");
}

string MalFn::print(bool) const {
  return "#<function>";
}

bool equal(MalType* a, MalType* b);

template <typename A, typename B>
bool equal_seq(A* a, B* b) {
  return std::equal(begin(a), end(a), begin(b), end(b), ::equal);
}

bool equal(MalType* a, MalType* b) {
  if (a == b)
    return true;
  if (typeid(*a) != typeid(*b))
    return false;
  return a->equal(b);
}
