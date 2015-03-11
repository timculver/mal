#include <iostream>
#include <string>

using namespace std;

string READ(string s) {
    return s;
}

string EVAL(string s) {
    return s;
}

string PRINT(string s) {
    return s;
}

string rep(string s) {
    return PRINT(EVAL(READ(s)));
}

int main(int argc, char* argv[]) {
    while (!cin.eof()) {
        cout << "> ";
        cout.flush();
        string line;
        getline(cin, line);
        cout << rep(line) << "\n";
    }
    return 0;
}
