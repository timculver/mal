#include "eval.hpp"

#include <iostream>
#include <sstream>
#include "types.hpp"

using namespace std;

MalType* eval_ast(MalType* form, Env* env) {
  if (auto symbol = dynamic_cast<MalSymbol*>(form)) {
    // This is the only difference between a keyword and a symbol, I believe.
    if (symbol->s[0] == ':')
      return symbol;
    return env->get(symbol);
  }
  if (auto list = dynamic_cast<MalList*>(form)) {
    vector<MalType*> v;
    for (auto p=list; p!=eol; p=p->cdr) {
      v.push_back(EVAL(p->car, env));
    }
    MalList* ret = eol;
    for (auto iter=v.rbegin(); iter!=v.rend(); iter++)
      ret = new MalList(*iter, ret);
    return ret;
  }
  if (auto vec = dynamic_cast<MalVector*>(form)) {
    vector<MalType*> v;
    for (auto element : vec->e)
      v.push_back(EVAL(element, env));
    return new MalVector(move(v));
  }
  if (auto hash = dynamic_cast<MalHash*>(form)) {
    RBTree<KeyValue> tree;
    forEach(hash->tree, [&tree, &env](const KeyValue& pair) {
      tree = tree.inserted(KeyValue{pair.key, EVAL(pair.value, env)});
    });
    return new MalHash(tree);
  }
  return form;
}

MalType* EVAL(MalType* form, Env* env) {
  if (MalList* list = dynamic_cast<MalList*>(form)) {
    MalType* head = list->car;
    MalList* rest = list->cdr;
    // Specials
    auto symbol = dynamic_cast<MalSymbol*>(head);
    if (symbol) {
      if (symbol->s == "quote") {
        return list->get(1);
      } else if (symbol->s == "def!") {
        auto name = list->get<MalSymbol>(1);
        auto value = EVAL(list->get(2), env);
        env->set(name, value);
        return value;
      } else if (symbol->s == "let*") {
        Env* local = new Env(env);
        auto bindings = rest->get(0);
        if (auto bindings_vec = dynamic_cast<MalVector*>(bindings)) {
          for (int ii=0; ii<bindings_vec->e.size(); ii+=2) {
            auto name = bindings_vec->get<MalSymbol>(ii);
            auto value = EVAL(bindings_vec->get(ii+1), local);
            local->set(name, value);
          }
        } else if (auto bindings_list = dynamic_cast<MalList*>(bindings)) {
          while (bindings_list != eol) {
            auto name = bindings_list->get<MalSymbol>(0);
            auto value = EVAL(bindings_list->get(1), local);
            local->set(name, value);
            bindings_list = bindings_list->cdr->cdr;
          }
        }
        return EVAL(rest->get(1), local);
      } else if (symbol->s == "do") {
        MalType* ret = nullptr;
        MalType* p = list->cdr;
        while (p != eol) {
          auto pcons = cast<MalList>(p);
          ret = EVAL(pcons->car, env);
          p = pcons->cdr;
        }
        return ret;
      } else if (symbol->s == "if") {
        auto pred = list->cdr;
        auto iftrue = pred->cdr;
        auto iffalse = iftrue->cdr;
        auto cond = EVAL(pred->car, env);
        if (cond && !dynamic_cast<MalFalse*>(cond) && !dynamic_cast<MalNil*>(cond))
          return EVAL(iftrue->car, env);
        else
          return iffalse == eol ? nil : EVAL(iffalse->car, env);
      } else if (symbol->s == "fn*") {
        auto bindings = rest->get<MalList>(0);
        auto body = rest->get(1);
        function<MalType*(MalList*)> f = [bindings, body, env](MalList* args) {
          Env* closure = new Env(env, bindings, args);
          return EVAL(body, closure);
        };
        return new MalFn(f);
      }
    }
    // Apply
    list = static_cast<MalList*>(eval_ast(list, env));
    MalFn* f = cast<MalFn>(list->car);
    return f->f(list->cdr);
  }
  return eval_ast(form, env);
}



