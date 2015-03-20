#include "eval.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include "types.hpp"

using namespace std;

MalType* eval_ast(MalType* form, Env* env) {
  if (auto keyword = match<MalKeyword>(form))
    return keyword;
  if (auto symbol = match<MalSymbol>(form))
    return env->get(symbol);
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

MalLambda* is_macro_call(MalType* ast, Env* env) {
  auto list = match<MalList>(ast);
  if (!list || list == eol)
    return nullptr;
  auto sym = match<MalSymbol>(list->car);
  if (!sym)
    return nullptr;
  auto fn = match<MalLambda>(env->lookup(sym));
  if (fn && fn->is_macro)
    return fn;
  return nullptr;
}

MalType* macroexpand(MalType* ast, Env* env) {
  while (auto lambda = is_macro_call(ast, env)) {
    ast = lambda->apply(cast<MalList>(ast)->cdr);
  }
  return ast;
}

bool is_pair(MalType* ast) {
  if (auto seq = match<MalSeq>(ast))
    return !seq->empty();
  return false;
}

MalType* quasiquote(MalType* ast) {
  static auto _concat = symbol("concat");
  static auto _cons = symbol("cons");
  if (!is_pair(ast))
    return ::list({_quote, ast});
  auto seq = cast<MalSeq>(ast);
  if (seq->first() == _unquote)
    return seq->nth(1);
  if (auto splice_unquoted = match_wrapped(_splice_unquote, seq->first()))
    return ::list({_concat, splice_unquoted, quasiquote(seq->rest())});
  return ::list({_cons, quasiquote(seq->first()), quasiquote(seq->rest())});
}

MalType* EVAL(MalType* form, Env* env) {
  static auto _def = symbol("def!");
  static auto _defmacro = symbol("defmacro!");
  static auto _macroexpand = symbol("macroexpand");
  static auto _let = symbol("let*");
  static auto _do = symbol("do");
  static auto _if = symbol("if");
  static auto _fn = symbol("fn*");
  static auto _try = symbol("try*");
  static auto _catch = symbol("catch*");
  while (true) {
    form = macroexpand(form, env);
    if (MalList* list = match<MalList>(form)) {
      MalType* head = list->car;
      MalList* rest = list->cdr;
      // Specials
      auto symbol = match<MalSymbol>(head);
      if (symbol) {
        if (symbol == _quote) {
          return rest->get(0);
        } else if (symbol == _quasiquote) {
          form = quasiquote(rest->get(0));
          continue;
        } else if (symbol == _unquote) {
          form = rest->get(0);
          continue;
        } else if (symbol == _def || symbol == _defmacro) {
          auto name = rest->get<MalSymbol>(0);
          auto value = EVAL(rest->get(1), env);
          if (symbol == _defmacro)
            cast<MalLambda>(value)->is_macro = true;
          env->set(name, value);
          return value;
        } else if (symbol == _macroexpand) {
          return macroexpand(rest->get(0), env);
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
        } else if (symbol == _try) {
          if (rest->count() != 2 || rest->get<MalList>(1)->get(0) != _catch)
            throw error("Incorrect try/catch syntax");
          auto catch_bind = cast<MalSymbol>(rest->get<MalList>(1)->get(1));
          auto catch_body = rest->get<MalList>(1)->get(2);
          try {
            return EVAL(rest->get(0), env);
          } catch (MalType* exc) {
            Env* catch_env = new Env(env, ::list({catch_bind}), ::list({exc}));
            return EVAL(catch_body, catch_env);
          }
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
          auto bindings = rest->get<MalSeq>(0);
          auto body = rest->get(1);
          if (rest->size() > 2)
            throw error("Too many arguments for " + _fn->s);
          return new MalLambda(bindings, body, env);
        }
      }
      // Apply
      list = static_cast<MalList*>(eval_ast(list, env));
      if (auto f = match<NativeFn>(list->car)) {
        return f->apply(list->cdr);
      } else if (auto lambda = match<MalLambda>(list->car)) {
        form = lambda->body;
        env = new Env(lambda->env, lambda->bindings, list->cdr);
        continue;
      } else {
        throw error("Expected Function");
      }
    }
    return eval_ast(form, env);
  }
}
