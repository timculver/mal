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
  while (true) {
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
          form = rest->get(1);
          env = local;
          continue;
        } else if (symbol->s == "do") {
          auto p = list->cdr;
          if (p == eol)
            return nil;
          while (p != eol) {
            if (p->cdr == eol) {
              form = p->car;
              break;
            }
            EVAL(p->car, env);
            p = p->cdr;
          }
          continue;
        } else if (symbol->s == "if") {
          auto pred = list->cdr;
          auto iftrue = pred->cdr;
          auto iffalse = iftrue->cdr;
          auto cond = EVAL(pred->car, env);
          if (cond && !dynamic_cast<MalFalse*>(cond) && !dynamic_cast<MalNil*>(cond))
            form = iftrue->car;
          else
            form = iffalse == eol ? nil : iffalse->car;
          continue;
        } else if (symbol->s == "fn*") {
          auto bindings = rest->get(0);
          auto body = rest->get(1);
          if (rest->size() > 2)
            throw Error{"Too many arguments for fn*"};
          return new MalLambda(bindings, body, env);
        }
      }
      // Apply
      list = static_cast<MalList*>(eval_ast(list, env));
      if (auto f = dynamic_cast<MalFn*>(list->car)) {
        return f->f(list->cdr);
      } else if (auto lambda = dynamic_cast<MalLambda*>(list->car)) {
        form = lambda->body;
        env = new Env(lambda->env, lambda->bindings, list->cdr);
        continue;
      } else {
        throw Error{"Expected Function"};
      }
    }
    return eval_ast(form, env);
  }
}



