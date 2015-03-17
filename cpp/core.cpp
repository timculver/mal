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
  env->set(symbol("list"), new NativeFn([](MalList* args) { return args; }));
  env->set(symbol("list?"), fn1([](MalType* arg) {
    return boolean(match<MalList>(arg)); }));
  
  // Sequences: functions that work on both lists and vectors
  env->set(symbol("empty?"), fn1<MalSeq>([](MalSeq* seq) -> MalType* {
    return boolean(seq->empty()); }));
  env->set(symbol("nth"), fn2<MalSeq, MalInt>([](MalSeq* seq, MalInt* n) {
    return seq->nth(n->v); }));
  env->set(symbol("count"), fn1<MalSeq>([](MalSeq* seq) {
    return new MalInt(seq->count()); }));
  env->set(symbol("cons"), fn2([](MalType* first, MalType* rest) {
    if (auto list = match<MalList>(rest))
      return cons(first, list);
    if (auto vec = match<MalVector>(rest))
      return cons(first, to_list(vec));
    return ::list({first, rest});
  }));
  env->set(symbol("concat"), new NativeFn([](MalList* sequences) {
    return concat(sequences); }));
  env->set(symbol("first"), fn1<MalSeq>([](MalSeq* seq) -> MalType* {
    return seq->first(); }));
  env->set(symbol("rest"), fn1<MalSeq>([](MalSeq* seq) -> MalSeq* {
    return seq->rest(); }));
  env->set(symbol("map"), fn2<MalFn, MalSeq>([](MalFn* f, MalSeq* seq) {
    if (auto list = match<MalList>(seq))
      return reduce([f](MalType* head, MalList* rest) -> MalList* {
        return cons(f->apply(::list({head})), rest); }, list);
    if (auto vec = match<MalVector>(seq))
      return reduce([f](MalType* head, MalType* prev) -> MalType* {
        return f->apply(::list({head})); }, vec, nullptr);
    throw Error{"Expected Sequence"};
  }));

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
  env->set(symbol("pr-str"), new NativeFn([](MalList* args) {
    stringstream s;
    for (auto p=args; p!=eol; p=p->cdr)
      s << (p==args ? "" : " ") << p->car->print(true);
    return new MalString(s.str());
  }));
  env->set(symbol("str"), new NativeFn([](MalList* args) {
    if (args == eol)
      return new MalString("");
    stringstream s;
    for (auto p=args; p!=eol; p=p->cdr)
      s << p->car->print(false);
    return new MalString(s.str());
  }));
  env->set(symbol("prn"), new NativeFn([](MalList* args) {
    for (auto p=args; p!=eol; p=p->cdr)
      cout << (p==args ? "" : " ") << p->car->print(true);
    cout << endl;
    return nil;
  }));
  env->set(symbol("println"), new NativeFn([](MalList* args) {
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
  
  // Exceptions
  env->set(symbol("throw"), fn1([](MalType* exc) -> MalType* {
    throw Error(exc); }));
    
  return env;
}
