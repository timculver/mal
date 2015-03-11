#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <string>
#include <functional>
#include <string>

#include "RBTree.h"

struct Error {
  Error(std::string message_) : message(message_) { }
  std::string message;
};

struct MalType;

template <typename T> inline std::string print_type();

bool equal(MalType* a, MalType* b);

struct MalType {
  virtual ~MalType() { }
  virtual std::string print(bool print_readably = true) const = 0;
private:
  // ::equal is the function to call; this is just part of the implementation.
  virtual bool equal(MalType*) const = 0;
  friend bool ::equal(MalType*, MalType*);
};
template<> inline std::string print_type<MalType>() { return "Object"; }

template <typename T = MalType>
T* cast(MalType* form) {
  auto ret = dynamic_cast<T*>(form);
  if (!ret)
    throw Error{"Expected " + print_type<T>() + ", found `" + form->print() + "`"};
  return ret;
}

struct MalEol;
extern MalEol* eol;

struct MalList : public MalType {
  MalList(MalType* car_, MalList* cdr_) : car(car_), cdr(cdr_) { }
  bool equal(MalType*) const;
  std::string print(bool print_readably = true) const;

  template <typename T = MalType>
  T* get(int ii) {
    MalList* p = this;
    while (ii-- > 0) {
      p = p->cdr;
    }
    return cast<T>(p->car);
  }
  int size();
  
  MalType *car;
  MalList *cdr;
};
template <> inline std::string print_type<MalList>() { return "List"; }

inline MalList* cdr(MalList* list) {
  return list->cdr;
}

struct MalEol : public MalList {
  MalEol() : MalList(nullptr, nullptr) { }
  bool equal(MalType*) const { return true; }
};

struct MalNil : public MalType {
  MalNil() { }
  bool equal(MalType*) const { return true; }
  std::string print(bool = true) const { return "nil"; }
};
extern MalNil* nil;

struct MalTrue : public MalType {
  MalTrue() { }
  bool equal(MalType*) const { return true; }
  std::string print(bool = true) const { return "true"; }
};
extern MalTrue* mtrue;

struct MalFalse : public MalType {
  MalFalse() { }
  bool equal(MalType*) const { return true; }
  std::string print(bool = true) const { return "false"; }
};
extern MalFalse* mfalse;

inline MalType* boolean(bool b) {
  if (b)
    return mtrue;
  else
    return mfalse;
}

struct MalSymbol : public MalType {
private:
  MalSymbol(std::string s_) : s(std::move(s_)) { }
  friend MalSymbol* symbol(const std::string&);
public:
  bool equal(MalType*) const {
    return false;
  }
  std::string print(bool print_readably = true) const;
  
  const std::string s;
};
template<> inline std::string print_type<MalSymbol>() { return "Symbol"; }

MalSymbol* symbol(const std::string&);

struct MalString : public MalType {
  MalString(std::string(s_)) : s(std::move(s_)) { }
  bool equal(MalType* other) const {
    return s == dynamic_cast<MalString*>(other)->s;
  }
  std::string print(bool print_readably = true) const;
  
  const std::string s;

  static std::string print_string(std::string, bool print_readably);
  static std::string unescape(std::string);
  static std::string escape(std::string);
};

struct MalInt : public MalType {
  MalInt(int v_) : v(v_) { }
  bool equal(MalType* other) const {
    return v == dynamic_cast<MalInt*>(other)->v;
  }
  std::string print(bool = true) const;
  
  const int v;
};
template<> inline std::string print_type<MalInt>() { return "Int"; }

struct MalFn : public MalType {
  template <typename F>
  MalFn(F f_) : f(std::move(f_)) { }

  bool equal(MalType*) const { return false; }
  std::string print(bool print_readably = true) const;
  const std::function<MalType*(MalList*)> f;
};
template<> inline std::string print_type<MalFn>() { return "Function"; }

// Strongly-typed function definitions use these templates to generate type-checking code
// and dispatch to a C++ function with the correct types.
template <typename T1 = MalType, typename F>
MalFn* fn1(F f) {
  return new MalFn([=](MalList* args) { return f(args->get<T1>(0)); });
}
template <typename T1 = MalType, typename T2 = MalType, typename F>
MalFn* fn2(F f) {
  return new MalFn([=](MalList* args) { return f(args->get<T1>(0), args->get<T2>(1)); });
}
template <typename T1 = MalType, typename T2 = MalType, typename T3 = MalType, typename F>
MalFn* fn3(F f) {
  return new MalFn([=](MalList* args) { return f(args->get<T1>(0), args->get<T2>(1), args->get<T3>(2)); });
}

struct MalVector : public MalType {
  MalVector(std::vector<MalType*> e_) : e(std::move(e_)) { }
  bool equal(MalType*) const;
  std::string print(bool print_readably = true) const;
  
  template <typename T = MalType>
  T* get(int ii) {
    if (ii >= e.size())
      throw Error{"Expected " + print_type<T>() + " at position " + std::to_string(ii) + " in list: `" + print() + "`"};
    auto ret = dynamic_cast<T*>(e[ii]);
    if (!ret)
      throw Error{"Expected " + print_type<T>() + " at position " + std::to_string(ii) + " in list: `" + print() + "`"};
    return ret;
  }
  
  const std::vector<MalType*> e;
};
template<> inline std::string print_type<MalVector>() { return "List"; }

inline MalVector* cdr(MalVector* list) {
  list->get(0);
  std::vector<MalType*> ret;
  ret.reserve(list->e.size() - 1);
  for (int ii=1; ii<list->e.size(); ii++)
    ret.push_back(list->e[ii]);
  return new MalVector(ret);
}

struct KeyValue {
  MalType* key; // must be string or symbol for now
  MalType* value;
  
  bool operator<(const KeyValue& other) const {
    auto str1 = dynamic_cast<MalString*>(key);
    auto sym1 = dynamic_cast<MalSymbol*>(key);
    auto str2 = dynamic_cast<MalString*>(other.key);
    auto sym2 = dynamic_cast<MalSymbol*>(other.key);
    if (str1 && str2)
      return str1->s < str2->s;
    if (sym1 && sym2)
      return sym1->s < sym2->s;
    return !!str1; // strings before symbols
  }
};

struct MalHash : public MalType {
  MalHash() { };
  MalHash(RBTree<KeyValue> tree_) : tree(std::move(tree_)) { }
  bool equal(MalType*) const { throw Error{"Unimplemented"}; }
  std::string print(bool) const;
  
  MalHash* assoc(MalType* key, MalType* value);
  
  const RBTree<KeyValue> tree;
};
template <> inline std::string print_type<MalHash>() { return "Hash"; }

struct ListIter {
  const MalList* p;
  ListIter(const MalList* p_) : p(p_) {}
  MalType* operator*() {
    return p->car;
  }
  ListIter& operator++() {
    p = p->cdr;
    return *this;
  }
};

inline ListIter begin(const MalList* list) {
  return list;
}

inline ListIter end(const MalList* list) {
  return eol;
}

#endif