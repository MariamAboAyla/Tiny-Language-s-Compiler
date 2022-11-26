#include "iostream"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "scanner.cpp"

using namespace std;

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

// program -> stmtseq
// stmtseq -> stmt { ; stmt }
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
// ifstmt -> if exp then stmtseq [ else stmtseq ] end
// repeatstmt -> repeat stmtseq until expr
// assignstmt -> identifier := expr
// readstmt -> read identifier
// writestmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> factor { (*|/) factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier
// /////////////////////////////////////////////////////////////////////////////////

// declaration for function "statement sequence"
TreeNode* stmtSeq(CompilerInfo *compiler, ParseInfo *parseInfo);


// function Expression:
// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* Expression(CompilerInfo *compiler, ParseInfo *parseInfo)
{
    TreeNode* node;



    return node;
}


// function to check if current-token-type == the wanted type
bool checkOnType(CompilerInfo *compiler, ParseInfo *parseInfo, TokenType tokenType) {
    return parseInfo->next_token.type == tokenType;
}

// ifstmt -> if exp then stmtseq [ else stmtseq ] end
TreeNode* ifStmt(CompilerInfo *compiler, ParseInfo *parseInfo)
{
    TreeNode* node;
    node->node_kind = IF_NODE;
    node->line_num = compiler->in_file.cur_line_num;

    GetNextToken(compiler,&parseInfo->next_token);
    node->child[0] = Expression(compiler, parseInfo);

    // check if "then" is there
    if(!checkOnType(compiler,parseInfo,THEN)) throw 0;
    GetNextToken(compiler,&parseInfo->next_token);
    node->child[1] = stmtSeq(compiler,parseInfo);

    GetNextToken(compiler,&parseInfo->next_token);

    // Else part - [Optional]
    if(checkOnType(compiler,parseInfo,ELSE))
    {
        GetNextToken(compiler,&parseInfo->next_token);
        node->child[2] = stmtSeq(compiler,parseInfo);
    }

    // End of If Statement
    if(!checkOnType(compiler,parseInfo,END)) throw 0;

    return node;
}

// repeatstmt -> repeat stmtseq until expr
TreeNode* repeatStmt(CompilerInfo *compiler, ParseInfo *parseInfo)
{
    TreeNode* node;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = REPEAT_NODE;
    GetNextToken(compiler,&parseInfo->next_token);
    node->child[0] = stmtSeq(compiler, parseInfo);

    GetNextToken(compiler, &parseInfo->next_token);

    //check whether it is "until"
    if(!checkOnType(compiler, parseInfo, UNTIL)) throw 0;

    GetNextToken(compiler, &parseInfo->next_token);

    node->child[1] = Expression(compiler, parseInfo);

    return node;
}

// assignstmt -> identifier := expr
TreeNode* assignStmt(CompilerInfo *compiler, ParseInfo *parseInfo)
{
    TreeNode* node;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = ASSIGN_NODE;
    AllocateAndCopy(&node->id,parseInfo->next_token.str);
    GetNextToken(compiler, &parseInfo->next_token);

    // check if the assignment operator is there
    if(!checkOnType(compiler, parseInfo, ASSIGN)) throw 0;
    GetNextToken(compiler,&parseInfo->next_token);
    node->child[0] = Expression(compiler, parseInfo);

    return node;
}

// readstmt -> read identifier
TreeNode* readStmt(CompilerInfo *compiler, ParseInfo *parseInfo)
{
    TreeNode* node;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = READ_NODE;
    GetNextToken(compiler,&parseInfo->next_token);

    checkOnType(compiler,parseInfo,ID)?AllocateAndCopy(&node->id,parseInfo->next_token.str):throw 0;

    GetNextToken(compiler,&parseInfo->next_token);
    return node;
}


// writestmt -> write expr
TreeNode* writeStmt(CompilerInfo *compiler, ParseInfo *parseInfo)
{
    TreeNode* node;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = WRITE_NODE;
    GetNextToken(compiler,&parseInfo->next_token);
    node->child[0] = Expression(compiler, parseInfo);
    return node;

}


// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
TreeNode* Stmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TokenType type = parseInfo->next_token.type;
    if (type == IF) {
        //if-statement
        return ifStmt(compiler, parseInfo);
    } else if (type == REPEAT) {
        //repeat-statement
        return repeatStmt(compiler, parseInfo);
    } else if (type == ID) {
        //assign-statement
        return assignStmt(compiler, parseInfo);
    } else if (type == READ) {
        //read-statement
        return readStmt(compiler, parseInfo);
    } else if (type == WRITE) {
        //write-statement
        return writeStmt(compiler, parseInfo);
    } else {
        throw 0;
    }

}

// stmtseq -> stmt { ; stmt }
TreeNode* stmtSeq(CompilerInfo *compiler, ParseInfo *parseInfo) {

    TreeNode *intialTree = Stmt(compiler, parseInfo);
    TreeNode *tmpTree = intialTree;

    // check if it is available statement :
    // ifstmt | repeatstmt | assignstmt | readstmt | writestmt

    //todo check this case later
    while (parseInfo->next_token.type == IF || parseInfo->next_token.type == REPEAT ||
           parseInfo->next_token.type == ID || parseInfo->next_token.type == READ ||
           parseInfo->next_token.type == WRITE || parseInfo->next_token.type==SEMI_COLON) {

        checkOnType(compiler, parseInfo, SEMI_COLON)?GetNextToken(compiler,&parseInfo->next_token):throw 0;
        TreeNode *nextTree = Stmt(compiler, parseInfo);
        tmpTree->sibling = nextTree;
        tmpTree = nextTree;
    }
    return intialTree;
}


// program -> stmtseq
TreeNode *Program(CompilerInfo *compiler) {
    TreeNode *parseTree;
    ParseInfo *parseInfo;

    GetNextToken(compiler, &parseInfo->next_token);

    // if file is empty or first token is error
    if (parseInfo->next_token.type == ENDFILE || parseInfo->next_token.type == ERROR) {
        compiler->out_file.Out("end of file");
    }

    // get statement sequence
    parseTree = stmtSeq(compiler, parseInfo);

    // when finish all statements
    if (parseInfo->next_token.type != ENDFILE) {
        compiler->out_file.Out("Error - didn't find end of file (EOF)");
        throw 0;
    }

    return parseTree;

}


// we print the returned tree from function: program