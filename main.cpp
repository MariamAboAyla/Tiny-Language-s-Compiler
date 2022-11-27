#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "scanner.cpp"
#include "parser.cpp"
using namespace std;
int main() {
    string filePath;
    cout<<"Please Enter File Path \n";
    getline(cin,filePath);
    CompilerInfo compiler(filePath.c_str(), "output.txt", "debug.txt");

    cout << "Start main()\n";

    TreeNode *parseTree = Program(&compiler);
    PrintTree(parseTree);
    fflush(NULL);

    cout << "End main()\n";

    return 0;
}
