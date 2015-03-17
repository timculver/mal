#ifndef ENV_HPP
#define ENV_HPP

#include <unordered_map>
#include <string>
#include <sstream>
#include "types.hpp"

using namespace std;


class Env {
public:
  Env(Env* outer_ = nullptr, MalSeq* binds = nullptr, MalList* exprs = nullptr)
      : outer(outer_) {
    static MalSymbol* splat = ::symbol("&");
    if (binds) {
      auto q = exprs;
      bool varargs = false, too_few_args = false;
      for_each(binds, [&](MalType* bind) {
        auto symbol = cast<MalSymbol>(bind);
        if (varargs) {
          set(symbol, q);
          return;
        }
        if (symbol == splat) {
          varargs = true;
          return;
        }
        if (q == eol) {
          // Keep going; we might find a "&" which should result in a different error message.
          too_few_args = true;
          return;
        }
        set(symbol, q->car);
        q = q->cdr;
      });
      bool too_many_args = !varargs && q != eol;
      if (too_few_args || too_many_args)
        throw funcall_error(binds->count() - (varargs ? 2 : 0), exprs->size(), varargs);
    }
  }
  void set(MalSymbol* k, MalType* v) {
    table[k->s] = v;
  }
  Env* find(MalSymbol* k) {
    if (table.find(k->s) != table.end())
      return this;
    else
      return outer ? outer->find(k) : nullptr;
  }
  MalType* get(MalSymbol* k) {
    auto result = lookup(k);
    if (result == nullptr)
      throw error("'" + k->s + "' not found");
    return result;
  }
  MalType* lookup(MalSymbol* k) {
    Env* env = find(k);
    if (env == nullptr)
      return nullptr;
    return env->table[k->s];
  }

private:
  inline static MalString* funcall_error(int num_bindings, int num_args, bool varargs) {
    // This case is worth the effort to put together a nice error message.
    stringstream err;
    err << "Function requires ";
    if (varargs)
      err << num_bindings << " or more";
    else
      err << num_bindings;
    err << " argument" << (num_bindings == 1 ? "" : "s") << "; got " << num_args;
    return error(err.str());
  }

  std::unordered_map<std::string, MalType*> table;
  Env* outer;
};

#endif