#include <iostream>
#include "syntaxTree.cpp"
#include "analyzer.cpp"
using namespace std;


int main() {

    CompilerInfo compilerInfo("input.txt", "output.txt", "debug.txt");
    // parseTree contains teh program's parse tree
    TreeNode* parseTree = Parse(&compilerInfo);
//    PrintTree(parseTree);
//    fflush(NULL);

    // 1) symbol table
    // 2) update type of data if non-void = done
    // 3) run simulation
//    SymbolTable symbolTable()

    return 0;
}
