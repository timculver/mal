#include "core.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

#include "env.hpp"
#include "reader.hpp"

using namespace std;

Env* core() {
  Env* env = new Env();

  // Symbol, string, true, false, nil
  env->set(symbol("symbol"), fn1<MalString>([](MalString* s) {
    return symbol(s->s); }));
  env->set(symbol("keyword"), fn1<MalString>([](MalString* s) {
    return symbol(":" + s->s); }));
  env->set(symbol("symbol?"), fn1<MalType>([](MalType* obj) -> MalType* {
    if (auto sym = match<MalSymbol>(obj))
      return boolean(!sym->is_keyword());
    return _false; }));
  env->set(symbol("keyword?"), fn1<MalType>([](MalType* obj) -> MalType* {
    if (auto sym = match<MalSymbol>(obj))
      return boolean(sym->is_keyword());
    return _false; }));
  env->set(symbol("true?"), fn1<MalType>([](MalType* obj) -> MalType* {
    return boolean(obj == _true); }));
  env->set(symbol("false?"), fn1<MalType>([](MalType* obj) -> MalType* {
    return boolean(obj == _false); }));
  env->set(symbol("nil?"), fn1<MalType>([](MalType* obj) -> MalType* {
    return boolean(obj == nil); }));

  // Integer math
  env->set(symbol("+"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v + b->v); }));
  env->set(symbol("*"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v * b->v); }));
  env->set(symbol("-"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v - b->v); }));
  env->set(symbol("/"), fn2<MalInt, MalInt>([](MalInt* a, MalInt* b) {
    return new MalInt(a->v / b->v); }));

  // Functions
  env->set(symbol("apply"), new NativeFn([](MalList* args) {
    auto f = cast<MalFn>(args->get(0));
    return f->apply(concat(cdr(args))); }));

  // Lists
  env->set(symbol("list"), new NativeFn([](MalList* args) { return args; }));
  env->set(symbol("list?"), fn1([](MalType* arg) {
    return boolean(match<MalList>(arg)); }));
  
  // Vectors
  env->set(symbol("vector"), new NativeFn([](MalList* args) {
    return to_vector(args); }));
  env->set(symbol("vector?"), fn1([](MalType* arg) {
    return boolean(match<MalVector>(arg)); }));

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
  env->set(symbol("map"), fn2<MalFn, MalSeq>([](MalFn* f, MalSeq* seq) -> MalList* {
    if (auto list = match<MalList>(seq))
      return reduce([f](MalType* head, MalList* rest) -> MalList* {
        return cons(f->apply(::list({head})), rest); }, list);
    if (auto vec = match<MalVector>(seq))
      return to_list(reduce([f](MalType* head, MalType* prev) -> MalType* {
        return f->apply(::list({head})); }, vec, nullptr));
    throw error("Expected Sequence");
  }));
  env->set(symbol("sequential?"), fn1([](MalType* arg) -> MalType* {
    if (auto seq = match<MalSeq>(arg)) {
      if (seq == nil)
        return _false;
      MalInt* prev = nullptr;
      bool sequential = true;
      for_each(seq, [&prev, &sequential](MalType* elem) {
        if (auto num = match<MalInt>(elem)) {
          if (prev && prev->v > num->v)
            sequential = false;
          prev = num;
        }
        // (sequential? ["z" "a"]) => true
      });
      return boolean(sequential);
    }
    return _false;
  }));
    
  // Hashes
  env->set(symbol("map?"), fn1([](MalType* arg) {
    return boolean(match<MalHash>(arg)); }));
  env->set(symbol("hash-map"), new NativeFn([](MalList* args) {
    MalHash* hash = new MalHash();
    for (auto p = args; p != eol; p = p->cdr->cdr)
      hash = hash->assoc(p->get(0), p->get(1));
    return hash; }));
  env->set(symbol("assoc"), new NativeFn([](MalList* args) {
    auto hash = cast<MalHash>(args->get(0));
    args = args->cdr;
    while (args != eol) {
      auto key = args->get(0);
      auto value = args->get(1);
      hash = hash->assoc(key, value);
      args = args->cdr->cdr;
    }
    return hash;
  }));
  env->set(symbol("dissoc"), fn2<MalHash, MalType>([](MalHash* hash, MalType* key) {
    return hash->dissoc(key);
  }));
  env->set(symbol("dissoc"), new NativeFn([](MalList* args) {
    auto hash = cast<MalHash>(args->get(0));
    return hash->dissoc_many(args->cdr);
  }));
  env->set(symbol("contains?"), fn2<MalType, MalType>([](MalType* hash_arg, MalType* key) -> MalType* {
    if (hash_arg == nil)
      return _false;
    return boolean(cast<MalHash>(hash_arg)->contains(key)); }));
  env->set(symbol("get"), fn2<MalType, MalType>([](MalType* hash_arg, MalType* key) -> MalType* {
    if (hash_arg == nil)
      return nil;
    return cast<MalHash>(hash_arg)->get(key); }));
  env->set(symbol("keys"), fn1<MalHash>([](MalHash* hash) {
    return hash->keys(); }));
  env->set(symbol("vals"), fn1<MalHash>([](MalHash* hash) {
    return hash->values(); }));
  
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
  
  // I/O
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
  env->set(symbol("read-string"), fn1<MalString>([](MalString* s) {
    return read_str(s->s); }));
  env->set(symbol("readline"), fn1<MalString>([](MalString* s) {
    cout << s->s << flush;
    string line;
    getline(cin, line);
    return new MalString(move(line)); }));
  env->set(symbol("slurp"), fn1<MalString>([](MalString* filename) {
    ifstream file(filename->s);
    stringstream sstr;
    sstr << file.rdbuf();
    return new MalString(sstr.str());
  }));
  
  // Exceptions
  env->set(symbol("throw"), fn1([](MalType* exc) -> MalType* {
    throw exc; }));
    
  return env;
}
