#include "eval.hpp"

#include <iostream>
#include <sstream>
#include "types.hpp"

using namespace std;

MalType* eval_ast(MalType* form, Env* env) {
  if (auto symbol = match<MalSymbol>(form)) {
    // I believe a keyword is identical to a symbol everywhere except here.
    if (symbol->s[0] == ':')
      return symbol;
    return env->get(symbol);
  }
  if (auto list = match<MalList>(form)) {
    return reduce([env](MalType* first, MalList* rest) {
      return cons(EVAL(first, env), rest); }, list);
  }
  if (auto vec = match<MalVector>(form)) {
    vector<MalType*> v;
    for (auto element : vec->e)
      v.push_back(EVAL(element, env));
    return new MalVector(move(v));
  }
  if (auto hash = match<MalHash>(form)) {
    RBTree<KeyValue> tree;
    forEach(hash->tree, [&tree, env](const KeyValue& pair) {
      tree = tree.inserted(KeyValue{pair.key, EVAL(pair.value, env)});
    });
    return new MalHash(tree);
  }
  return form;
}

// Check for the form `(sym x)` and return x if a match.
MalType* match_wrapped(MalSymbol* sym, MalType* form) {
  if (auto list = match<MalList>(form))
    if (list != eol && list->cdr != eol && list->car == sym)
      return list->cdr->car;
  return nullptr;
}

MalType* quasiquote(MalType* ast, Env* env) {
  if (auto unquoted = match_wrapped(_unquote, ast))
    return EVAL(unquoted, env);
  if (auto list = match<MalList>(ast))
    return reduce([env](MalType* first, MalList* rest) {
      if (auto unquoted = match_wrapped(_unquote, first))
        return cons(EVAL(unquoted, env), rest);
      if (auto splice_unquoted = match_wrapped(_splice_unquote, first))
        return concat2(cast<MalList>(EVAL(splice_unquoted, env)), rest);
      return cons(first, rest);
    }, list);
  if (auto vec = match<MalVector>(ast))
    return quasiquote(to_list(vec), env);
  return ast;
}

MalType* EVAL(MalType* form, Env* env) {
  static auto _def = symbol("def!");
  static auto _let = symbol("let*");
  static auto _do = symbol("do");
  static auto _if = symbol("if");
  static auto _fn = symbol("fn*");
  while (true) {
    if (MalList* list = match<MalList>(form)) {
      MalType* head = list->car;
      MalList* rest = list->cdr;
      // Specials
      auto symbol = match<MalSymbol>(head);
      if (symbol) {
        if (symbol == _quote) {
          return rest->get(0);
        } else if (symbol == _quasiquote) {
          return quasiquote(rest->get(0), env);
        } else if (symbol == _unquote) {
          form = rest->get(0);
          continue;
        } else if (symbol == _def) {
          auto name = rest->get<MalSymbol>(0);
          auto value = EVAL(rest->get(1), env);
          env->set(name, value);
          return value;
        } else if (symbol == _let) {
          Env* local = new Env(env);
          auto bindings = rest->get(0);
          if (auto bindings_vec = match<MalVector>(bindings)) {
            for (int ii=0; ii<bindings_vec->e.size(); ii+=2) {
              auto name = bindings_vec->get<MalSymbol>(ii);
              auto value = EVAL(bindings_vec->get(ii+1), local);
              local->set(name, value);
            }
          } else if (auto bindings_list = match<MalList>(bindings)) {
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
        } else if (symbol == _do) {
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
        } else if (symbol == _if) {
          auto cond = EVAL(rest->get(0), env);
          if (!match<MalFalse>(cond) && !match<MalNil>(cond)) {
            form = rest->get(1);
          } else {
            auto iffalse = rest->cdr->cdr;
            form = iffalse == eol ? nil : iffalse->car;
          }
          continue;
        } else if (symbol == _fn) {
          auto bindings = rest->get(0);
          auto body = rest->get(1);
          if (rest->size() > 2)
            throw Error{"Too many arguments for " + _fn->s};
          return new MalLambda(bindings, body, env);
        }
      }
      // Apply
      list = static_cast<MalList*>(eval_ast(list, env));
      if (auto f = match<MalFn>(list->car)) {
        return f->f(list->cdr);
      } else if (auto lambda = match<MalLambda>(list->car)) {
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
