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
  return form ? form->print() : "";
}

string rep(string s, Env* env) {
  return PRINT(EVAL(READ(s), env));
}

int main(int argc, char* argv[]) {
  Env* repl_env = core();
  rep("(def! not (fn* (a) (if a false true)))", repl_env);
  
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
