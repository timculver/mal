#include "core.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

#include "env.hpp"
#include "reader.hpp"

using namespace std;

Env* core() {
  Env* env = new Env();

  // Integer math
  env->set(symbol("+"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v + b->v); }));
  env->set(symbol("*"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v * b->v); }));
  env->set(symbol("-"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v - b->v); }));
  env->set(symbol("/"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v / b->v); }));

  // Lists
  env->set(symbol("list"), new MalFn([](MalList* args) { return args; }));
  env->set(symbol("list?"), fn1<MalType>([](MalType* arg) {
    return boolean(match<MalList>(arg)); }));
  
  // Sequences: functions that work on both lists and vectors
  env->set(symbol("empty?"), fn1([](MalType* arg) -> MalType* {
    if (arg == nil)
      return _true;
    if (auto list = match<MalList>(arg))
      return boolean(list == eol);
    if (auto vec = match<MalVector>(arg))
      return boolean(vec->e.empty());
    throw Error{"Expected Sequence"};
  }));
  env->set(symbol("count"), fn1([](MalType* obj) {
    if (obj == nil)
      return new MalInt(0);
    if (auto list = match<MalList>(obj))
      return new MalInt(list->size());
    if (auto vec = match<MalVector>(obj))
      return new MalInt((int)vec->e.size());
    throw Error{"Expected Sequence"};
  }));
  env->set(symbol("cons"), fn2([](MalType* first, MalType* rest) {
    if (auto list = match<MalList>(rest))
      return cons(first, list);
    if (auto vec = match<MalVector>(rest))
      return cons(first, vec);
    throw Error{"Expected Sequence"};
  }));
  env->set(symbol("concat"), new MalFn([](MalList* sequences) {
    return concat(sequences); }));

  // Hashes
  env->set(symbol("assoc"), fn3<MalHash, MalType, MalType>([](MalHash* hash, MalType* key, MalType* value) {
    return hash->assoc(key, value);
  }));
  
  // Comparisons
  env->set(symbol("="), fn2([](MalType* a, MalType* b) {
    return boolean(::equal(a, b)); }));
  env->set(symbol("<"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return boolean(a->v < b->v); }));
  env->set(symbol("<="), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return boolean(a->v <= b->v); }));
  env->set(symbol(">"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return boolean(a->v > b->v); }));
  env->set(symbol(">="), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return boolean(a->v >= b->v); }));
  
  // Printing
  env->set(symbol("pr-str"), new MalFn([](MalList* args) {
    stringstream s;
    for (auto p=args; p!=eol; p=p->cdr)
      s << (p==args ? "" : " ") << p->car->print(true);
    return new MalString(s.str());
  }));
  env->set(symbol("str"), new MalFn([](MalList* args) {
    if (args == eol)
      return new MalString("");
    stringstream s;
    for (auto p=args; p!=eol; p=p->cdr)
      s << p->car->print(false);
    return new MalString(s.str());
  }));
  env->set(symbol("prn"), new MalFn([](MalList* args) {
    for (auto p=args; p!=eol; p=p->cdr)
      cout << (p==args ? "" : " ") << p->car->print(true);
    cout << endl;
    return nil;
  }));
  env->set(symbol("println"), new MalFn([](MalList* args) {
    for (auto p=args; p!=eol; p=p->cdr)
      cout << (p==args ? "" : " ") << p->car->print(false);
    cout << endl;
    return nil;
  }));

  // Files
  env->set(symbol("read-string"), fn1<MalString>([](MalString* s) {
    return read_str(s->s); }));
  env->set(symbol("slurp"), fn1<MalString>([](MalString* filename) {
    ifstream file(filename->s);
    stringstream sstr;
    sstr << file.rdbuf();
    return new MalString(sstr.str());
  }));
    
  return env;
}
