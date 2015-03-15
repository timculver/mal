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

MalList* concat2(MalList* a, MalList* b) {
  if (a == eol)
    return b;
  return cons(car(a), concat2(cdr(a), b));
}

MalList* concat(MalList* sequences) {
  if (sequences == eol)
    return eol;
  auto first = car(sequences);
  auto rest = cdr(sequences);
  if (auto vec = dynamic_cast<MalVector*>(first))
    return concat2(to_list(vec), concat(rest));
  return concat2(cast<MalList>(first), concat(rest));
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

string MalLambda::print(bool) const {
  return "#<lambda>";
}

bool equal(MalType* a, MalType* b);

bool equal_list_vector(MalList* list, MalVector* vector) {
  auto p = list;
  auto q = vector->e.begin();
  while (true) {
    bool p_end = (p == eol);
    bool q_end = (q == vector->e.end());
    if (p_end && q_end)
      return true;
    if (p_end || q_end)
      return false;
    if (!equal(p->car, *q))
      return false;
    p = p->cdr;
    q++;
  }
  return true;
}

bool equal(MalType* a, MalType* b) {
  // Reference equality
  if (a == b)
    return true;
  // Simple single-dispatch cases
  if (typeid(*a) == typeid(*b))
    return a->equal(b);
  // Double-dispatch cases
  if (dynamic_cast<MalList*>(a) && dynamic_cast<MalVector*>(b))
    return equal_list_vector(static_cast<MalList*>(a), static_cast<MalVector*>(b));
  if (dynamic_cast<MalList*>(b) && dynamic_cast<MalVector*>(a))
    return equal_list_vector(static_cast<MalList*>(b), static_cast<MalVector*>(a));
  return false;
}
