#ifndef ENV_HPP
#define ENV_HPP

#include <unordered_map>
#include <string>
#include <sstream>
#include "types.hpp"

using namespace std;

class Env {
public:
  Env(Env* outer_ = nullptr, MalList* binds = nullptr, MalList* exprs = nullptr)
    : outer(outer_) {
    static MalSymbol* splat = ::symbol("&");
    if (binds) {
      auto p = binds, q = exprs;
      bool varargs = false, too_few_args = false;
      while (p != eol) {
        auto symbol = p->get<MalSymbol>(0);
        if (symbol == splat) {
          varargs = true;
          auto varargs_symbol = p->get<MalSymbol>(1);
          set(varargs_symbol, q);
          break;
        }
        if (q == eol) {
          // Keep going; we might find a "&" which should result in a different error message.
          too_few_args = true;
          p = p->cdr;
          continue;
        }
        set(cast<MalSymbol>(p->car), q->car);
        p = p->cdr;
        q = q->cdr;
      }
      bool too_many_args = !varargs && q != eol;
      if (too_few_args || too_many_args)
        throw Error{funcall_error(binds->size() - (varargs ? 2 : 0), exprs->size(), varargs)};
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
    Env* env = find(k);
    if (env == nullptr)
      throw Error{"Not found: `" + k->s + "`"};
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