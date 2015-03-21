#include <iostream>
#include <string>

#include "env.hpp"
#include "reader.hpp"
#include "eval.hpp"
#include "core.hpp"

using namespace std;

MalType* READ(string s) {
  return read_str(s);
}

string PRINT(MalType* form) {
  return form ? form->print() : "#<nullptr>";
}

string rep(string s, Env* env) {
  return PRINT(EVAL(READ(s), env));
}

int main(int argc, char* argv[]) {
  Env* repl_env = core();
  rep("(def! *host-language* \"c++\")", repl_env);
  rep("(def! not (fn* (a) (if a false true)))", repl_env);
  repl_env->set(symbol("eval"), fn1([repl_env](MalType* form) { return EVAL(form, repl_env); }));
  rep("(def! load-file (fn* (f) (eval (read-string (str \"(do \" (slurp f) \")\")))))", repl_env);
  rep("(defmacro! cond (fn* (& xs) (if (> (count xs) 0) (list 'if (first xs) (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms to cond\")) (cons 'cond (rest (rest xs)))))))", repl_env);
  rep("(defmacro! or (fn* (& xs) (if (empty? xs) nil (if (= 1 (count xs)) (first xs) `(let* (or_FIXME ~(first xs)) (if or_FIXME or_FIXME (or ~@(rest xs))))))))", repl_env);

  MalList* argv_list = eol;
  for (int ii = argc - 1; ii >= 2; ii--)
    argv_list = new MalList(new MalString(string(argv[ii])), argv_list);
  repl_env->set(symbol("*ARGV*"), argv_list);
  
  if (argc >= 2) {
    try {
      rep("(load-file \"" + escape(string(argv[1])) + "\")", repl_env);
    } catch (MalType* error) {
      cout << error->print(false) << "\n";
      return 1;
    }
    return 0;
  }

  rep("(println (str \"Mal [\" *host-language* \"]\"))", repl_env);
  
  while (!cin.eof()) {
    cout << "user> ";
    cout.flush();
    string line;
    getline(cin, line);
    try {
      string out = rep(line, repl_env);
      if (!out.empty())
        cout << out << "\n";
    } catch (MalType* error) {
      cout << error->print(false) << "\n";
    }
  }
  return 0;
}
