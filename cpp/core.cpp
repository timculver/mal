#include "core.hpp"

#include <iostream>
#include <sstream>

#include "env.hpp"

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

  // Sequence utilities
  env->set(symbol("list"), new MalFn([](MalList* args) { return args; }));
  env->set(symbol("list?"), fn1<MalType>([](MalType* arg) {
    return boolean(dynamic_cast<MalList*>(arg)); }));
  env->set(symbol("empty?"), fn1([](MalType* arg) {
    if (auto list = dynamic_cast<MalList*>(arg))
      return boolean(list == eol);
    if (auto vec = dynamic_cast<MalVector*>(arg))
      return boolean(vec->e.empty());
    throw Error{"Expected Sequence"};
  }));
  env->set(symbol("count"), fn1([](MalType* obj) {
    if (auto list = dynamic_cast<MalList*>(obj))
      return new MalInt(list->size());
    if (auto vec = dynamic_cast<MalVector*>(obj))
      return new MalInt((int)vec->e.size());
    if (obj == nil)
      return new MalInt(0);
    throw Error{"Expected Sequence"};
  }));
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


  return env;
}