#include "types.hpp"

#include <sstream>
#include <regex>
#include <unordered_map>

#include "env.hpp"
#include "eval.hpp"

using namespace std;

Meta::Meta() : meta(nil) { }

bool MalVector::equal_impl(MalType* other_object) const {
  auto other = static_cast<MalVector*>(other_object);
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

MalSeq* MalVector::rest() {
  if (e.empty())
    return eol;
  else
    return cdr(to_list(this));
}

MalEol* eol = new MalEol();
MalNil* nil = new MalNil();
MalTrue* _true = new MalTrue();
MalFalse* _false = new MalFalse();

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

bool MalList::equal_impl(MalType* other_obj) const {
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
  if (auto list = match<MalList>(first))
    return concat2(list, concat(rest));
  if (auto vec = match<MalVector>(first))
    return concat2(to_list(vec), concat(rest));
  return cons(first, concat(rest));
}

string Atom::print(bool print_readably) const {
  stringstream s;
  s << "(atom " << ref->print() << ")";
  return s.str();
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

MalHash* MalHash::assoc(HashKey* key, MalType* value) {
  return new MalHash(tree.inserted(KeyValue{key, value}));
}

MalHash* MalHash::dissoc(HashKey* key) {
  // TODO: Implement delete properly, or find a real HAMT impl.
  RBTree<KeyValue> newtree;
  forEach(tree, [&key, &newtree](const KeyValue& kv) {
    if (!equal(kv.key, key))
      newtree = newtree.inserted(kv);
  });
  return new MalHash(newtree);
}

MalHash* MalHash::dissoc_many(MalList* keys) {
  for (auto p = keys; p != eol; p = p->cdr)
    if (!match<HashKey>(p->car))
      throw error("Expected String or Symbol or Keyword, got " + p->car->print());
  // TODO: Implement delete properly, or find a real HAMT impl.
  RBTree<KeyValue> newtree;
  forEach(tree, [&keys, &newtree](const KeyValue& kv) {
    bool to_delete = false;
    for (auto p = keys; p != eol; p = p->cdr) {
      if (equal(kv.key, p->car)) {
        to_delete = true;
        break;
      }
    }
    if (!to_delete)
      newtree = newtree.inserted(kv);
  });
  return new MalHash(newtree);
}

MalList* MalHash::keys() {
  MalList* keys = eol;
  forEach(tree, [&keys](const KeyValue& kv) {
    keys = cons(kv.key, keys);
  });
  return keys;
}

MalList* MalHash::values() {
  MalList* values = eol;
  forEach(tree, [&values](const KeyValue& kv) {
    values = cons(kv.value, values);
  });
  return values;
}

bool MalHash::contains(HashKey* key) {
  return tree.member(KeyValue{key, nullptr});
}

MalType* MalHash::get(HashKey* key) {
  auto kv = tree.find(KeyValue{key, nullptr});
  return kv.key ? kv.value : nil;
}

string MalInt::print(bool) const {
  stringstream s;
  s << v;
  return s.str();
}

string MalSymbol::print(bool) const {
  return s;
}

MalSymbol::MalSymbol(std::string s_) : s(std::move(s_)) {
  if (s.empty())
    throw error("Empty string can't be converted to Symbol");
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
MalSymbol* _quote = symbol("quote");
MalSymbol* _quasiquote = symbol("quasiquote");
MalSymbol* _unquote = symbol("unquote");
MalSymbol* _splice_unquote = symbol("splice-unquote");


string MalKeyword::print(bool) const {
  return s;
}

MalKeyword::MalKeyword(std::string s_) : s(std::move(s_)) {
  if (s.empty())
    throw error("Empty string can't be converted to Keyword");
}

MalKeyword* keyword(const string& s) {
  static unordered_map<string, MalKeyword*> table;
  auto it = table.find(s);
  if (it != table.end())
    return it->second;
  auto k = new MalKeyword(s);
  table[s] = k;
  return k;
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

string NativeFn::print(bool) const {
  return "#<function>";
}

string MalLambda::print(bool) const {
  return is_macro ? "#<macro>" : "#<lambda>";
}

MalType* MalLambda::apply(MalList* args) {
  auto exec_env = new Env(env, bindings, args);
  return EVAL(body, exec_env);
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
    return a->equal_impl(b);
  // Double-dispatch cases
  if (dynamic_cast<MalList*>(a) && dynamic_cast<MalVector*>(b))
    return equal_list_vector(static_cast<MalList*>(a), static_cast<MalVector*>(b));
  if (dynamic_cast<MalList*>(b) && dynamic_cast<MalVector*>(a))
    return equal_list_vector(static_cast<MalList*>(b), static_cast<MalVector*>(a));
  return false;
}
