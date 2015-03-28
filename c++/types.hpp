#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <string>
#include <functional>
#include <string>

#include "RBTree.h"

class MalType;
class MalList;
class MalSeq;
class MalString;
class Env;

// Create an error message for throwing.
MalString* error(std::string s);

template <typename T> T* cast(MalType* form);

bool equal(MalType* a, MalType* b);

class MalType {
public:
  virtual ~MalType() { }
  virtual std::string print(bool print_readably = true) const = 0;
private:
  // Implementations of equal_impl may safely static_cast to their own type.
  // They can also assume that the argument is different from `this`.
  virtual bool equal_impl(MalType*) const = 0;
  friend bool ::equal(MalType*, MalType*);
};

// Mixin for types supporting metadata.
class Meta {
public:
  Meta();
  MalType* meta() { return metadata; }
  virtual Meta* copy() = 0;
  Meta* with_meta(MalType* newmeta) {
    auto newobj = copy();
    newobj->metadata = newmeta;
    return newobj;
  }

private:
  MalType* metadata;
};

// true, false

class MalTrue : public MalType {
public:
  MalTrue() { }
  bool equal_impl(MalType*) const { return true; }
  std::string print(bool = true) const { return "true"; }
};
extern MalTrue* _true;

class MalFalse : public MalType {
public:
  MalFalse() { }
  bool equal_impl(MalType*) const { return true; }
  std::string print(bool = true) const { return "false"; }
};
extern MalFalse* _false;

// Convert a C++ bool to a Mal true/false value.
inline MalType* boolean(bool b) {
  if (b)
    return _true;
  else
    return _false;
}

// Symbol, keyword, string, int

// Base class for objects that are allowed as hash-map keys.
class HashKey : public MalType {
public:
  virtual const std::string& get_string() = 0;
};

class MalSymbol : public HashKey {
  MalSymbol(std::string s_);
  friend MalSymbol* symbol(const std::string&);
  friend MalSymbol* gensym();
public:
  bool equal_impl(MalType*) const override { return false; }
  std::string print(bool print_readably = true) const override;
  const std::string& get_string() override { return s; }

private:
  std::string s;
};

MalSymbol* symbol(const std::string&);
MalSymbol* gensym();

extern MalSymbol* _quote;
extern MalSymbol* _quasiquote;
extern MalSymbol* _unquote;
extern MalSymbol* _splice_unquote;

class MalKeyword : public HashKey {
  MalKeyword(std::string s_);
  friend MalKeyword* keyword(const std::string&);
public:
  bool equal_impl(MalType*) const override { return false; }
  std::string print(bool print_readably = true) const override;
  const std::string& get_string() override { return s; }
  
private:
  const std::string s;
};

MalKeyword* keyword(const std::string&);

class MalString : public HashKey {
public:
  MalString(std::string(s_)) : s(std::move(s_)) { }
  bool equal_impl(MalType* other) const override {
    return s == static_cast<MalString*>(other)->s;
  }
  std::string print(bool print_readably = true) const;
  const std::string& get_string() override { return s; }

  const std::string s;
};

std::string print_string(std::string, bool print_readably);
std::string unescape(std::string);
std::string escape(std::string);

// Number

class Number : public MalType {
public:
  Number(double v_) : v(v_) { }
  bool equal_impl(MalType* other) const override {
    return v == static_cast<Number*>(other)->v;
  }
  std::string print(bool = true) const override;
  
  const double v;
};

// Functions (native and lambda)

class MalFn : public MalType {
public:
  virtual MalType* apply(MalList* args) = 0;
};

class NativeFn : public MalFn, public Meta {
public:
  template <typename F>
  NativeFn(F f_) : f(std::move(f_)) { }
  bool equal_impl(MalType*) const override { return false; }
  std::string print(bool print_readably = true) const override;
  MalType* apply(MalList* args) override { return f(args); }
  Meta* copy() override { return new NativeFn(f); }

private:
  const std::function<MalType*(MalList*)> f;
};

class MalLambda : public MalFn, public Meta {
public:
  MalLambda(MalSeq* bindings_, MalType* body_, Env* env_, bool is_macro_ = false)
    : bindings(bindings_), body(body_), env(env_), is_macro(is_macro_) { }
  bool equal_impl(MalType*) const override { return false; }
  std::string print(bool print_readably = true) const override;
  MalType* apply(MalList* args) override;
  Meta* copy() override { return new MalLambda(bindings, body, env, is_macro); }
  
  MalSeq* bindings;
  MalType* body;
  Env* env;
  bool is_macro;
};

// Sequence

class MalSeq : public MalType {
public:
  virtual bool empty() = 0;
  virtual int count() = 0;
  virtual MalType* first() = 0;
  virtual MalSeq* rest() = 0;
  virtual MalType* nth(int n) = 0;
};

// Nil (empty sequence)

// `nil` is not the empty list; it is distinct from '(). In sequence contexts
// it is an empty sequence, to be treated like '() or []. It is also the
// return value of a function with only side effects.
class MalNil : public MalSeq {
public:
  MalNil() { }
  bool equal_impl(MalType*) const override { return true; }
  bool empty() override { return true; }
  int count() override { return 0; }
  MalType* first() override { return this; }
  MalSeq* rest() override { return this; }
  MalType* nth(int n) override { throw error("Empty Sequence"); }
  std::string print(bool = true) const override { return "nil"; }
};
extern MalNil* nil;

// List

// Clojure/Mal 'nil' has special behavior in many places and isn't suitable
// for end-of-list. I introduce 'eol' as the end-of-list sentinel. I don't
// represent any Mal value with nullptr.
class MalList;
extern MalList* eol;

// Meta is on every list node, which is a bit wasteful.
class MalList : public MalSeq, public Meta {
public:
  MalList(MalType* car_, MalList* cdr_) : car(car_), cdr(cdr_) { }
  bool equal_impl(MalType*) const override;
  std::string print(bool print_readably = true) const override;
  bool empty() override { return this == eol; }
  int count() override { return size(); }
  MalType* first() override { return empty() ? nil : get(0); }
  MalSeq* rest() override { if (empty()) return eol; return cdr; }
  MalType* nth(int n) override { return get(n); }
  Meta* copy() override { return new MalList(car, cdr); }
  template <typename T = MalType> T* get(int ii);
  int size();
  template <typename F> void for_each(F&& f);
  
  MalType* const car;
  MalList* const cdr;
};

inline MalList* cons(MalType* first, MalList* rest) {
  return new MalList(first, rest);
}

// Concatenate a list of sequences.
MalList* concat(MalList* lists);

// Concatenate two lists.
MalList* concat2(MalList* a, MalList* b);

template <typename T>
T* MalList::get(int ii) {
  MalList* p = this;
  while (ii-- > 0 && p != eol)
    p = p->cdr;
  if (p == eol)
    throw error("Index out of range");
  return cast<T>(p->car);
}

template <typename F> void MalList::for_each(F&& f) {
  for (auto p = this; p != eol; p = p->cdr)
    f(p->car);
}

template <typename F>
MalList* reduce(F&& f, MalList* list) {
  std::vector<MalType*> v;
  for (auto p = list; p != eol; p = p->cdr)
    v.push_back(p->car);
  MalList* q = eol;
  for (auto iter=v.rbegin(); iter!=v.rend(); iter++)
    q = f(*iter, q);
  return q;
}

// Vector

class MalVector : public MalSeq, public Meta {
public:
  MalVector(std::vector<MalType*> e_) : e(std::move(e_)) { }
  bool equal_impl(MalType*) const override;
  std::string print(bool print_readably = true) const override;
  bool empty() override { return e.empty(); }
  int count() override { return (int)e.size(); }
  MalType* first() override { return e.empty() ? nil : get(0); }
  MalSeq* rest() override;
  MalType* nth(int n) override { return get(n); }
  Meta* copy() override { return new MalVector(e); }
  template <typename T = MalType>
  T* get(int ii) {
    if (ii >= e.size())
      throw error(std::string("Expected ") + typeid(T).name() + " at position " + std::to_string(ii) + " in list: `" + print() + "`");
    auto ret = dynamic_cast<T*>(e[ii]);
    if (!ret)
      throw error(std::string("Expected ") + typeid(T).name() + " at position " + std::to_string(ii) + " in list: `" + print() + "`");
    return ret;
  }
  int size() {
    return int(e.size());
  }
  template <typename F>
  void for_each(F&& f) {
    for (auto i : e)
      f(i);
  }
  
  const std::vector<MalType*> e;
};

inline MalList* to_list(MalVector* vec) {
  MalList* list = eol;
  for (auto iter = vec->e.rbegin(); iter != vec->e.rend(); ++iter)
    list = cons(*iter, list);
  return list;
}

inline MalVector* to_vector(MalList* list) {
  std::vector<MalType*> v;
  for (auto p = list; p != eol; p = p->cdr)
    v.push_back(p->car);
  return new MalVector(std::move(v));
}

inline MalList* list(std::initializer_list<MalType*> init) {
  std::vector<MalType*> vec(init);
  return to_list(new MalVector(vec));
}

inline MalVector* cdr(MalVector* list) {
  list->get(0);
  std::vector<MalType*> ret;
  ret.reserve(list->e.size() - 1);
  for (int ii=1; ii<list->e.size(); ii++)
    ret.push_back(list->e[ii]);
  return new MalVector(ret);
}

template <typename F>
MalVector* reduce(F&& f, MalVector* vec, MalType* base) {
  std::vector<MalType*> v;
  for (auto obj : vec->e) {
    base = f(obj, base);
    v.push_back(base);
  }
  return new MalVector(move(v));
}

// Hash

struct KeyValue {
  HashKey* key; // Internally, key=nullptr is a sentinel in RBTree::find().
                // I should have used RBMap.
  MalType* value;
  
  bool operator<(const KeyValue& other) const {
    if (key == other.key)
      return false;
    if (key->get_string() != other.key->get_string())
      return key->get_string() < other.key->get_string();
    // Unstable type comparison
    return typeid(*key).hash_code() < typeid(*other.key).hash_code();
  }
};

class MalHash : public MalType, public Meta {
public:
  MalHash() { };
  MalHash(RBTree<KeyValue> tree_) : tree(std::move(tree_)) { }
  bool equal_impl(MalType*) const override { throw error("Unimplemented"); }
  std::string print(bool) const override;
  Meta* copy() override { return new MalHash(tree); }
  MalHash* assoc(HashKey* key, MalType* value);
  MalHash* dissoc(HashKey* key);
  MalHash* dissoc_many(MalList* keys);
  MalType* get(HashKey* key);
  bool contains(HashKey* key);
  MalList* keys();
  MalList* values();
  
  const RBTree<KeyValue> tree;
};

// Atom

class Atom : public MalType {
public:
  Atom(MalType* ref_) : ref(ref_) { }
  bool equal_impl(MalType*) const override { return false; }
  std::string print(bool) const override;
  
  MalType* ref;
};

// Utilities

// Attempt to convert to T*; throw a descriptive error on failure.
template <typename T = MalType>
T* cast(MalType* form) {
  auto ret = dynamic_cast<T*>(form);
  if (!ret)
    throw error(std::string("Expected ") + typeid(T).name() + ", found `" + form->print() + "`");
  return ret;
}

// Attempt to convert to T*; return nullptr on failure.
template <typename T>
T* match(MalType* form) {
  return dynamic_cast<T*>(form);
}

template <typename F> void for_each(MalSeq* seq, F&& f) {
  if (auto list = match<MalList>(seq))
    return list->for_each(f);
  if (auto vec = match<MalVector>(seq))
    return vec->for_each(f);
  throw error("Expected Sequence");
}

// Strongly-typed function definitions use these templates to generate type-checking code
// and dispatch to a C++ function with the correct types.
template <typename T1 = MalType, typename F>
NativeFn* fn1(F f) {
  return new NativeFn([=](MalList* args) { return f(args->get<T1>(0)); });
}
template <typename T1 = MalType, typename T2 = MalType, typename F>
NativeFn* fn2(F f) {
  return new NativeFn([=](MalList* args) { return f(args->get<T1>(0), args->get<T2>(1)); });
}
template <typename T1 = MalType, typename T2 = MalType, typename T3 = MalType, typename F>
NativeFn* fn3(F f) {
  return new NativeFn([=](MalList* args) { return f(args->get<T1>(0), args->get<T2>(1), args->get<T3>(2)); });
}

inline MalString* error(std::string s) {
  return new MalString(move(s));
}

#endif