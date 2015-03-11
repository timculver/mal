#include <iostream>
#include <string>

#include "env.hpp"
#include "reader.hpp"
#include "eval.hpp"

using namespace std;

MalType* READ(string s) {
  return read_str(s);
}

string PRINT(MalType* form) {
  return form ? form->print() : "";
}

string rep(string s, Env& env) {
    return PRINT(EVAL(READ(s), env));
}

MalType* divide(MalInt* a, MalInt* b) { return new MalInt(a->v / b->v); }
std::function<MalType*(MalInt*,MalInt*)> divide_fn = divide;

template <typename T1, typename T2, typename F>
MalFn* fn(F f) {
  std::function<MalType*(T1*, T2*)> f2 = f;
  return new MalFn(move(f2));
}

int main(int argc, char* argv[]) {
  Env repl_env;
  
  repl_env.set(new MalSymbol("+"), new MalFn([](MalList* args) {
    int result = 0;
    for (int ii=1; ii<args->e.size(); ii++)
      result += dynamic_cast<MalInt*>(args->e[ii])->v;
    return new MalInt(result);
  }));
  
  repl_env.set(new MalSymbol("*"), new MalFn([](MalList* args) {
    int result = 1;
    for (int ii=1; ii<args->e.size(); ii++)
      result *= dynamic_cast<MalInt*>(args->e[ii])->v;
    return new MalInt(result);
  }));
  
  repl_env.set(new MalSymbol("-"), fn<MalInt, MalInt>([](MalInt* a, MalInt* b) { return new MalInt(a->v - b->v); }));
  repl_env.set(new MalSymbol("/"), fn<MalInt, MalInt>([](MalInt* a, MalInt* b) { return new MalInt(a->v / b->v); }));
  
    while (!cin.eof()) {
        cout << "user> ";
        cout.flush();
        string line;
        getline(cin, line);
        try {
          string out = rep(line, repl_env);
          if (!out.empty())
            cout << out << "\n";
        } catch(Error& error) {
          cout << error.message << "\n";
        }
    }
    return 0;
}
