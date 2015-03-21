#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <string>
#include <functional>
#include <string>

#include "RBTree.h"

struct MalType;
struct MalList;
struct MalSeq;
struct MalString;
class Env;

// Create an error message for throwing.
MalString* error(std::string s);

template <typename T> inline std::string print_type();

bool equal(MalType* a, MalType* b);

struct MalType {
  virtual ~MalType() { }
  virtual std::string print(bool print_readably = true) const = 0;
private:
  // Implementations of equal_impl may safely static_cast to their own type.
  // They can also assume that the argument is different from `this`.
  virtual bool equal_impl(MalType*) const = 0;
  friend bool ::equal(MalType*, MalType*);
};
template<> inline std::string print_type<MalType>() { return "Object"; }

template <typename T = MalType>
T* cast(MalType* form) {
  auto ret = dynamic_cast<T*>(form);
  if (!ret)
    throw error("Expected " + print_type<T>() + ", found `" + form->print() + "`");
  return ret;
}

template <typename T>
T* match(MalType* form) {
  return dynamic_cast<T*>(form);
}

struct Meta {
  Meta();
  
  virtual Meta* copy() = 0;
  
  Meta* with_meta(MalType* newmeta) {
    auto newobj = copy();
    newobj->meta = newmeta;
    return newobj;
  }

  MalType* meta;
};
template<> inline std::string print_type<Meta>() { return "Function/List/Vector/Hash"; }

// true, false

struct MalTrue : public MalType {
  MalTrue() { }
  bool equal_impl(MalType*) const { return true; }
  std::string print(bool = true) const { return "true"; }
};
extern MalTrue* _true;

struct MalFalse : public MalType {
  MalFalse() { }
  bool equal_impl(MalType*) const { return true; }
  std::string print(bool = true) const { return "false"; }
};
extern MalFalse* _false;

inline MalType* boolean(bool b) {
  if (b)
    return _true;
  else
    return _false;
}

// Symbol, string, int

struct HashKey : public MalType {
  virtual const std::string& get_string() = 0;
};
template<> inline std::string print_type<HashKey>() { return "String or Symbol or Keyword"; }

struct MalSymbol : public HashKey {
private:
  MalSymbol(std::string s_);
  friend MalSymbol* symbol(const std::string&);
  friend MalSymbol* gensym();
public:
  // MalType overrides
  bool equal_impl(MalType*) const { return false; }
  std::string print(bool print_readably = true) const;
  
  // HashKey override
  const std::string& get_string() { return s; }

  const std::string s;
};
template<> inline std::string print_type<MalSymbol>() { return "Symbol"; }

MalSymbol* symbol(const std::string&);
extern MalSymbol* _quote;
extern MalSymbol* _quasiquote;
extern MalSymbol* _unquote;
extern MalSymbol* _splice_unquote;
MalSymbol* gensym();

struct MalKeyword : public HashKey {
private:
  MalKeyword(std::string s_);
  friend MalKeyword* keyword(const std::string&);
public:
  // MalType overrides
  bool equal_impl(MalType*) const { return false; }
  std::string print(bool print_readably = true) const;
  
  // HashKey override
  const std::string& get_string() { return s; }
  
  const std::string s;
};
template<> inline std::string print_type<MalKeyword>() { return "Keyword"; }

MalKeyword* keyword(const std::string&);

struct MalString : public HashKey {
  MalString(std::string(s_)) : s(std::move(s_)) { }
  
  // MalType overrides
  bool equal_impl(MalType* other) const {
    return s == static_cast<MalString*>(other)->s;
  }
  std::string print(bool print_readably = true) const;
  
  // HashKey override
  const std::string& get_string() { return s; }
  
  const std::string s;

  static std::string print_string(std::string, bool print_readably);
  static std::string unescape(std::string);
  static std::string escape(std::string);
};
template<> inline std::string print_type<MalString>() { return "String"; }

struct Number : public MalType {
  Number(double v_) : v(v_) { }
  bool equal_impl(MalType* other) const {
    return v == static_cast<Number*>(other)->v;
  }
  std::string print(bool = true) const;
  
  const double v;
};
template<> inline std::string print_type<Number>() { return "Int"; }

// Functions (native and lambda)

struct MalFn : public MalType {
  virtual MalType* apply(MalList* args) = 0;
};
template<> inline std::string print_type<MalFn>() { return "Function"; }

struct NativeFn : public MalFn, public Meta {
  template <typename F>
  NativeFn(F f_) : f(std::move(f_)) { }

  bool equal_impl(MalType*) const { return false; }
  std::string print(bool print_readably = true) const;

  MalType* apply(MalList* args) { return f(args); }
  
  Meta* copy() { return new NativeFn(f); }

private:
  const std::function<MalType*(MalList*)> f;
};
template<> inline std::string print_type<NativeFn>() { return "Function"; }

struct MalLambda : public MalFn, public Meta {
  MalLambda(MalSeq* bindings_, MalType* body_, Env* env_, bool is_macro_ = false)
    : bindings(bindings_), body(body_), env(env_), is_macro(is_macro_) { }
  
  // MalType overrides
  bool equal_impl(MalType*) const { return false; }
  std::string print(bool print_readably = true) const;

  // MalFn overrides
  MalType* apply(MalList* args);

  // Meta overrides
  Meta* copy() { return new MalLambda(bindings, body, env, is_macro); }
  
  MalSeq* bindings;
  MalType* body;
  Env* env;
  bool is_macro;
};
template<> inline std::string print_type<MalLambda>() { return "Lambda/Macro"; }

// Sequence

struct MalSeq : public MalType {
  virtual bool empty() = 0;
  virtual int count() = 0;
  virtual MalType* first() = 0;
  virtual MalSeq* rest() = 0;
  virtual MalType* nth(int n) = 0;
};
template <> inline std::string print_type<MalSeq>() { return "Sequence"; }

// Nil (empty sequence)

// `nil` is not the empty list; it is distinct from '(). In sequence contexts
// it is an empty sequence, to be treated like '() or []. It is also the
// return value of a function with only side effects.
struct MalNil : public MalSeq {
  MalNil() { }
  
  // MalType overrides
  bool equal_impl(MalType*) const { return true; }
  
  // MalSeq overrides
  bool empty() { return true; }
  int count() { return 0; }
  MalType* first() { return this; }
  MalSeq* rest() { return this; }
  MalType* nth(int n) { throw error("Empty Sequence"); }
  
  std::string print(bool = true) const { return "nil"; }
};
extern MalNil* nil;


// List

struct MalList;

// Clojure/Mal 'nil' has special behavior in many places and isn't suitable
// for end-of-list. I introduce 'eol' as the end-of-list sentinel. I don't
// represent any Mal value with nullptr.
extern MalList* eol;

// Meta is on every list node. Not very nice.
struct MalList : public MalSeq, public Meta {
  MalList(MalType* car_, MalList* cdr_) : car(car_), cdr(cdr_) { }
  
  // MalType overrides
  bool equal_impl(MalType*) const;
  std::string print(bool print_readably = true) const;

  // MalSeq overrides
  bool empty() { return this == eol; }
  int count() { return size(); }
  MalType* first() { return empty() ? nil : get(0); }
  MalSeq* rest() { if (empty()) return eol; return cdr; }
  MalType* nth(int n) { return get(n); }
  
  // Meta overrides
  Meta* copy() { return new MalList(car, cdr); }
  
  // Methods
  template <typename T = MalType> T* get(int ii);
  int size();
  template <typename F> void for_each(F&& f);
  
  MalType *car;
  MalList *cdr;
};
template <> inline std::string print_type<MalList>() { return "List"; }

inline MalType* car(MalList* list) {
  return list->car;
}

inline MalList* cdr(MalList* list) {
  return list->cdr;
}

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

struct MalVector : public MalSeq, public Meta {
  MalVector(std::vector<MalType*> e_) : e(std::move(e_)) { }
  
  // MalType overrides
  bool equal_impl(MalType*) const;
  std::string print(bool print_readably = true) const;
  
  // MalSeq overrides
  bool empty() { return e.empty(); }
  int count() { return (int)e.size(); }
  MalType* first() { return e.empty() ? nil : get(0); }
  MalSeq* rest();
  MalType* nth(int n) { return get(n); }
  
  // Meta overrides
  Meta* copy() { return new MalVector(e); }
  
  // Methods
  template <typename T = MalType>
  T* get(int ii) {
    if (ii >= e.size())
      throw error("Expected " + print_type<T>() + " at position " + std::to_string(ii) + " in list: `" + print() + "`");
    auto ret = dynamic_cast<T*>(e[ii]);
    if (!ret)
      throw error("Expected " + print_type<T>() + " at position " + std::to_string(ii) + " in list: `" + print() + "`");
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
template<> inline std::string print_type<MalVector>() { return "List"; }

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

struct MalHash : public MalType, public Meta {
  MalHash() { };
  MalHash(RBTree<KeyValue> tree_) : tree(std::move(tree_)) { }

  bool equal_impl(MalType*) const { throw error("Unimplemented"); }
  std::string print(bool) const;

  Meta* copy() { return new MalHash(tree); }
  
  MalHash* assoc(HashKey* key, MalType* value);
  MalHash* dissoc(HashKey* key);
  MalHash* dissoc_many(MalList* keys);
  MalType* get(HashKey* key);
  bool contains(HashKey* key);
  MalList* keys();
  MalList* values();
  
  const RBTree<KeyValue> tree;
};
template <> inline std::string print_type<MalHash>() { return "Hash"; }

// Atom

struct Atom : public MalType {
  Atom(MalType* ref_) : ref(ref_) { }
  
  bool equal_impl(MalType*) const { return false; }
  std::string print(bool) const;
  
  MalType* ref;
};
template <> inline std::string print_type<Atom>() { return "Atom"; }

// Utilities

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