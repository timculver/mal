#include <iostream>
#include <string>

#include "reader.hpp"

using namespace std;

MalType* READ(string s) {
  return read_str(s);
}

MalType* EVAL(MalType* s) {
  return s;
}

string PRINT(MalType* form) {
  return form ? form->print() : "";
}

string rep(string s) {
    return PRINT(EVAL(READ(s)));
}

int main(int argc, char* argv[]) {
    while (!cin.eof()) {
        cout << "user> ";
        cout.flush();
        string line;
        getline(cin, line);
        try {
          string out = rep(line);
          if (!out.empty())
            cout << out << "\n";
        } catch(Error& error) {
          cout << "Parse error\n";
        }
    }
    return 0;
}
