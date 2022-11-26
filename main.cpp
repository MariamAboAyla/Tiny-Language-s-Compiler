#include <iostream>
#include "scanner.cpp"
#include "parser.cpp"
#include <vector>
using namespace std;

void addNode(TreeNode* parseTree, ParseInfo* parseInfo)
{
    Token token = parseInfo->next_token;
    parseTree->oper = token.type;

    //to assign the node-kind (IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
    //    OPER_NODE, NUM_NODE, ID_NODE)


}

void StartScanner(CompilerInfo* pci, TreeNode* parseTree)
{
    Token token;
    ParseInfo parseInfo;
    vector<Token> nodeToken;
    while(true)
    {
        nodeToken.clear();

        GetNextToken(pci, &token);
        printf("[%d] %s (%s)\n", pci->in_file.cur_line_num, token.str, TokenTypeStr[token.type]);
        fflush(NULL);

        // send to parse tree the token
        parseInfo.next_token = token;
        addNode(parseTree, &parseInfo);
        if(token.type == IF)
        {

        }else if (token.type == REPEAT)
        {

        }else if (token.type == READ)
        {

        }else if (token.type == WRITE)
        {

        }else if (token.type == ASSIGN)
        {

        }else if (token.type == NUM)
        {

        }else if (token.type == ID)
        {

        }else if (token.type == EQUAL || token.type ==  LESS_THAN || token.type == PLUS
               || token.type == MINUS || token.type ==  TIMES || token.type ==  DIVIDE || token.type ==  POWER)
        {

        }

        if(token.type==ENDFILE || token.type==ERROR) break;
    }
}



int main() {


    CompilerInfo compiler("input.txt", "output.txt", "debug.txt");

    TreeNode parseTree;
    StartScanner(&compiler, &parseTree);



    return 0;
}
