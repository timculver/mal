#ifndef ENV_HPP
#define ENV_HPP

#include <unordered_map>
#include <string>
#include <sstream>
#include "types.hpp"

using namespace std;


class Env {
public:
  Env(Env* outer_ = nullptr, MalType* binds = nullptr, MalList* exprs = nullptr)
    : outer(outer_) {
    if (auto binds_list = match<MalList>(binds))
      set_bindings(binds_list, exprs);
    else if (auto binds_vec = match<MalVector>(binds))
      set_bindings(binds_vec, exprs);
    else if (binds)
      throw Error{"Expected Seqence"};
  }
  
  template <typename Seq>
  void set_bindings(Seq& binds, MalList* exprs) {
    static MalSymbol* splat = ::symbol("&");
    auto q = exprs;
    bool varargs = false, too_few_args = false;
    binds->for_each([&](MalType* bind) {
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
      throw Error{funcall_error(binds->size() - (varargs ? 2 : 0), exprs->size(), varargs)};
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
      throw Error{"Not found: `" + k->s + "`"};
    return result;
  }
  MalType* lookup(MalSymbol* k) {
    Env* env = find(k);
    if (env == nullptr)
      return nullptr;
    return env->table[k->s];
  }

private:
  inline static string funcall_error(int num_bindings, int num_args, bool varargs) {
    // This case is worth the effort to put together a nice error message.
    stringstream err;
    err << "Function requires ";
    if (varargs)
      err << num_bindings << " or more";
    else
      err << num_bindings;
    err << " argument" << (num_bindings == 1 ? "" : "s") << "; got " << num_args;
    return err.str();
  }

  std::unordered_map<std::string, MalType*> table;
  Env* outer;
};

#endif