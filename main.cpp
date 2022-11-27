#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace std;

/*
{ Sample program
  in TINY language
  compute factorial
}

read x; {input an integer}
if 0<x then {compute only if x>=1}
  fact:=1;
  repeat
    fact := fact * x;
    x:=x-1
  until x=0;
  write fact {output factorial}
end
*/

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

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

bool StartsWith(const char *a, const char *b) {
    int nb = strlen(b);
    return strncmp(a, b, nb) == 0;
}

void Copy(char *a, const char *b, int n = 0) {
    if (n > 0) {
        strncpy(a, b, n);
        a[n] = 0;
    }
    else strcpy(a, b);
}

void AllocateAndCopy(char **a, const char *b) {
    if (b == 0) {
        *a = 0;
        return;
    }
    int n = strlen(b);
    *a = new char[n + 1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile {
    FILE *file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char *str) {
        file = 0;
        if (str) file = fopen(str, "r");
        cur_line_size = 0;
        cur_ind = 0;
        cur_line_num = 0;
    }

    ~InFile() { if (file) fclose(file); }

    void SkipSpaces() {
        while (cur_ind < cur_line_size) {
            char ch = line_buf[cur_ind];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char *str) {
        while (true) {
            SkipSpaces();
            while (cur_ind >= cur_line_size) {
                if (!GetNewLine()) return false;
                SkipSpaces();
            }

            if (StartsWith(&line_buf[cur_ind], str)) {
                cur_ind += strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine() {
        cur_ind = 0;
        line_buf[0] = 0;
        if (!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size = strlen(line_buf);
        if (cur_line_size == 0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char *GetNextTokenStr() {
        SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; SkipSpaces();}
        return &line_buf[cur_ind];
    }

    void Advance(int num) {
        cur_ind += num;
    }
};

struct OutFile {
    FILE *file;

    OutFile(const char *str) {
        file = 0;
        if (str) file = fopen(str, "w");
    }

    ~OutFile() { if (file) fclose(file); }

    void Out(const char *s) {
        fprintf(file, "%s\n", s);
        fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo {
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char *in_str, const char *out_str, const char *debug_str)
            : in_file(in_str), out_file(out_str), debug_file(debug_str) {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType {
    IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
    ASSIGN, EQUAL, LESS_THAN,
    PLUS, MINUS, TIMES, DIVIDE, POWER,
    SEMI_COLON,
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    ID, NUM,
    ENDFILE, ERROR
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *TokenTypeStr[] =
        {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
        };

struct Token {
    TokenType type;
    char str[MAX_TOKEN_LEN + 1];

    Token() {
        str[0] = 0;
        type = ERROR;
    }

    Token(TokenType _type, const char *_str) {
        type = _type;
        Copy(str, _str);
    }
};

const Token reserved_words[] =
        {
                Token(IF, "if"),
                Token(THEN, "then"),
                Token(ELSE, "else"),
                Token(END, "end"),
                Token(REPEAT, "repeat"),
                Token(UNTIL, "until"),
                Token(READ, "read"),
                Token(WRITE, "write")
        };
const int num_reserved_words = sizeof(reserved_words) / sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[] =
        {
                Token(ASSIGN, ":="),
                Token(EQUAL, "="),
                Token(LESS_THAN, "<"),
                Token(PLUS, "+"),
                Token(MINUS, "-"),
                Token(TIMES, "*"),
                Token(DIVIDE, "/"),
                Token(POWER, "^"),
                Token(SEMI_COLON, ";"),
                Token(LEFT_PAREN, "("),
                Token(RIGHT_PAREN, ")"),
                Token(LEFT_BRACE, "{"),
                Token(RIGHT_BRACE, "}")
        };
const int num_symbolic_tokens = sizeof(symbolic_tokens) / sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch) { return (ch >= '0' && ch <= '9'); }

inline bool IsLetter(char ch) { return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')); }

inline bool IsLetterOrUnderscore(char ch) { return (IsLetter(ch) || ch == '_'); }

void GetNextToken(CompilerInfo *pci, Token *ptoken) {
    ptoken->type = ERROR;
    ptoken->str[0] = 0;

    int i;
    char *s = pci->in_file.GetNextTokenStr();
    if (!s) {
        ptoken->type = ENDFILE;
        ptoken->str[0] = 0;
        return;
    }

    for (i = 0; i < num_symbolic_tokens; i++) {
        if (StartsWith(s, symbolic_tokens[i].str))
            break;
    }

    if (i < num_symbolic_tokens) {
        if (symbolic_tokens[i].type == LEFT_BRACE) {
            pci->in_file.Advance(strlen(symbolic_tokens[i].str));
            if (!pci->in_file.SkipUpto(symbolic_tokens[i + 1].str)) return;
            return GetNextToken(pci, ptoken);
        }
        ptoken->type = symbolic_tokens[i].type;
        Copy(ptoken->str, symbolic_tokens[i].str);
    } else if (IsDigit(s[0])) {
        int j = 1;
        while (IsDigit(s[j])) j++;

        ptoken->type = NUM;
        Copy(ptoken->str, s, j);
    } else if (IsLetterOrUnderscore(s[0])) {
        int j = 1;
        while (IsLetterOrUnderscore(s[j])) j++;

        ptoken->type = ID;
        Copy(ptoken->str, s, j);

        for (i = 0; i < num_reserved_words; i++) {
            if (Equals(ptoken->str, reserved_words[i].str)) {
                ptoken->type = reserved_words[i].type;
                break;
            }
        }
    }

    int len = strlen(ptoken->str);
    if (len > 0) pci->in_file.Advance(len);

}

////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

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

enum NodeKind {
    IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
    OPER_NODE, NUM_NODE, ID_NODE
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *NodeKindStr[] =
        {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID"
        };

enum ExprDataType {
    VOID, INTEGER, BOOLEAN
};

// Used for debugging only /////////////////////////////////////////////////////////
const char *ExprDataTypeStr[] =
        {
                "Void", "Integer", "Boolean"
        };

#define MAX_CHILDREN 3

struct TreeNode {
    TreeNode *child[MAX_CHILDREN];
    TreeNode *sibling; // used for sibling statements only

    NodeKind node_kind;

    union {
        TokenType oper;
        int num;
        char *id;
    }; // defined for expression/int/identifier only
    ExprDataType expr_data_type; // defined for expression/int/identifier only

    int line_num;

    TreeNode() {
        int i;
        for (i = 0; i < MAX_CHILDREN; i++) child[i] = 0;
        sibling = 0;
        expr_data_type = VOID;
    }
};

struct ParseInfo {
    Token next_token;
};

void PrintTree(TreeNode *node, int sh = 0) {
    int i, NSH = 3;
    for (i = 0; i < sh; i++) printf(" ");

    printf("[%s]", NodeKindStr[node->node_kind]);

    if (node->node_kind == OPER_NODE) printf("[%s]", TokenTypeStr[node->oper]);
    else if (node->node_kind == NUM_NODE) printf("[%d]", node->num);
    else if (node->node_kind == ID_NODE || node->node_kind == READ_NODE || node->node_kind == ASSIGN_NODE)
        printf("[%s]", node->id);

    if (node->expr_data_type != VOID) printf("[%s]", ExprDataTypeStr[node->expr_data_type]);

    printf("\n");

    for (i = 0; i < MAX_CHILDREN; i++) if (node->child[i]) PrintTree(node->child[i], sh + NSH);
    if (node->sibling) PrintTree(node->sibling, sh);
}


////////////////////////////////////////////


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

// declaration for some functions
TreeNode *stmtSeq(CompilerInfo *compiler, ParseInfo *parseInfo);
TreeNode *mathExpression(CompilerInfo *compiler, ParseInfo *parseInfo);


// function to check if current-token-type == the wanted type
bool checkOnType(CompilerInfo *compiler, ParseInfo *parseInfo, TokenType tokenType) {
    return parseInfo->next_token.type == tokenType;
}


// New-Expression function:
// newexpr -> ( mathexpr ) | identifier | number
TreeNode *newExpression(CompilerInfo *compiler, ParseInfo *parseInfo) {

    TreeNode *node;
    node->line_num = compiler->in_file.cur_line_num;

    if (checkOnType(compiler, parseInfo, LEFT_BRACE)) {
        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);
        node = mathExpression(compiler, parseInfo);

        // get next token: to check if it will take right bracket or error
        if (!checkOnType(compiler, parseInfo, RIGHT_BRACE))
            throw 0;

        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);

    } else if (checkOnType(compiler, parseInfo, ID)) {
        node->node_kind = ID_NODE;

        GetNextToken(compiler, &parseInfo->next_token);

        // to add the value of the ID to the node
        AllocateAndCopy(&node->id, parseInfo->next_token.str);

        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);

    } else if (checkOnType(compiler, parseInfo, NUM)) {
        node->node_kind = NUM_NODE;

        GetNextToken(compiler, &parseInfo->next_token);

        // to add the value of the num to the node
        node->num = stoi(parseInfo->next_token.str);

        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);

    } else {
        throw 0;
    }
    return node;
}


// Factor function
// factor -> newexpr { ^ newexpr }    right associative
TreeNode *Factor(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *leftNode = newExpression(compiler, parseInfo);
    TreeNode *node = leftNode;

    while (parseInfo->next_token.type == POWER) {
        TreeNode *rootNode = new TreeNode;
        rootNode->node_kind = OPER_NODE;
        rootNode->line_num = compiler->in_file.cur_line_num;
        rootNode->oper = parseInfo->next_token.type;

        // assign the left-node to the OPER node
        rootNode->child[0] = node;

        // get the next token -> right node of the OPER_NODE
        GetNextToken(compiler, &parseInfo->next_token);

        // assign the right-node to the OPER node
        TreeNode *rightNode = Factor(compiler, parseInfo);
        rootNode->child[1] = rightNode;

//        node = rightNode;
        return rootNode;
    }
    return node;
}


// Term function
// term -> factor { (*|/) factor }    left associative
TreeNode *Term(CompilerInfo *compiler, ParseInfo *parseInfo) {

    TreeNode *leftNode = Factor(compiler, parseInfo);
    TreeNode *node = leftNode;

    while (parseInfo->next_token.type == TIMES || parseInfo->next_token.type == DIVIDE) {
        TreeNode *rootNode = new TreeNode;
        rootNode->node_kind = OPER_NODE;
        rootNode->line_num = compiler->in_file.cur_line_num;
        rootNode->oper = parseInfo->next_token.type;

        // assign the left-node to the OPER_node
        rootNode->child[0] = node;

        // get the next token -> right node of the OPER_NODE
        GetNextToken(compiler, &parseInfo->next_token);

        // assign the right-node to the OPER node
        TreeNode *rightNode = Factor(compiler, parseInfo);
        rootNode->child[1] = rightNode;

        node = rootNode;

    }
    return node;

}


// MathExpression fuction
// mathexpr -> term { (+|-) term }    left associative
TreeNode *mathExpression(CompilerInfo *compiler, ParseInfo *parseInfo) {

    TreeNode *leftNode = Term(compiler, parseInfo);
    TreeNode *node = leftNode;

    while (parseInfo->next_token.type == PLUS || parseInfo->next_token.type == MINUS) {
        TreeNode *rootNode = new TreeNode();
        rootNode->node_kind = OPER_NODE;
        rootNode->line_num = compiler->in_file.cur_line_num;
        rootNode->oper = parseInfo->next_token.type;

        // assign the left-node to the OPER node
        rootNode->child[0] = node;

        // get the next token -> right node of the OPER_NODE
        GetNextToken(compiler, &parseInfo->next_token);

        // assign the right-node to the OPER node
        TreeNode *rightNode = Term(compiler, parseInfo);
        rootNode->child[1] = rightNode;

        node = rootNode;

    }
    return node;
}

// function Expression:
// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode *Expression(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *leftNode = mathExpression(compiler, parseInfo);
    TreeNode *node = leftNode;
    if (parseInfo->next_token.type == EQUAL || parseInfo->next_token.type == LESS_THAN) {
        TreeNode *rootNode = new TreeNode;
        rootNode->node_kind = OPER_NODE;
        rootNode->line_num = compiler->in_file.cur_line_num;
        rootNode->oper = parseInfo->next_token.type;

        // assign the left-node to the OPER node
        rootNode->child[0] = leftNode;

        // get the next token -> right node of the OPER_NODE
        GetNextToken(compiler, &parseInfo->next_token);

        // assign the right-node to the OPER node
        TreeNode *rightNode = mathExpression(compiler, parseInfo);
        rootNode->child[1] = rightNode;

        node = rootNode;
    }

    return node;
}


// ifstmt -> if exp then stmtseq [ else stmtseq ] end
TreeNode *ifStmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *node = new TreeNode();
    node->node_kind = IF_NODE;
    node->line_num = compiler->in_file.cur_line_num;

    GetNextToken(compiler, &parseInfo->next_token);
    node->child[0] = Expression(compiler, parseInfo);

    // check if "then" is there
    if (!checkOnType(compiler, parseInfo, THEN)) throw 0;
    GetNextToken(compiler, &parseInfo->next_token);
    node->child[1] = stmtSeq(compiler, parseInfo);

    GetNextToken(compiler, &parseInfo->next_token);

    // Else part - [Optional]
    if (checkOnType(compiler, parseInfo, ELSE)) {
        GetNextToken(compiler, &parseInfo->next_token);
        node->child[2] = stmtSeq(compiler, parseInfo);
    }

    // End of If Statement
    if (!checkOnType(compiler, parseInfo, END)) throw 0;

    return node;
}

// repeatstmt -> repeat stmtseq until expr
TreeNode *repeatStmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *node = new TreeNode;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = REPEAT_NODE;
    GetNextToken(compiler, &parseInfo->next_token);
    node->child[0] = stmtSeq(compiler, parseInfo);

    GetNextToken(compiler, &parseInfo->next_token);

    //check whether it is "until"
    if (!checkOnType(compiler, parseInfo, UNTIL)) throw 0;

    GetNextToken(compiler, &parseInfo->next_token);

    node->child[1] = Expression(compiler, parseInfo);

    return node;
}

// assignstmt -> identifier := expr
TreeNode *assignStmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *node = new TreeNode;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = ASSIGN_NODE;
    AllocateAndCopy(&node->id, parseInfo->next_token.str);
    GetNextToken(compiler, &parseInfo->next_token);

    // check if the assignment operator is there
    if (!checkOnType(compiler, parseInfo, ASSIGN)) throw 0;
    GetNextToken(compiler, &parseInfo->next_token);
    node->child[0] = Expression(compiler, parseInfo);

    return node;
}

// readstmt -> read identifier
TreeNode *readStmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *node = new TreeNode;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = READ_NODE;
    GetNextToken(compiler, &parseInfo->next_token);

    checkOnType(compiler, parseInfo, ID) ? AllocateAndCopy(&node->id, parseInfo->next_token.str) : throw 0;

    GetNextToken(compiler, &parseInfo->next_token);
    return node;
}


// writestmt -> write expr
TreeNode *writeStmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *node = new TreeNode;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = WRITE_NODE;
    GetNextToken(compiler, &parseInfo->next_token);
    node->child[0] = Expression(compiler, parseInfo);
    return node;

}


// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
TreeNode *Stmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
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
TreeNode *stmtSeq(CompilerInfo *compiler, ParseInfo *parseInfo) {

    TreeNode *intialTree = Stmt(compiler, parseInfo);
    TreeNode *tmpTree = intialTree;

    // check if it is available statement :
    // ifstmt | repeatstmt | assignstmt | readstmt | writestmt
    while (parseInfo->next_token.type == IF || parseInfo->next_token.type == REPEAT ||
           parseInfo->next_token.type == ID || parseInfo->next_token.type == READ ||
           parseInfo->next_token.type == WRITE || parseInfo->next_token.type == SEMI_COLON) {

        checkOnType(compiler, parseInfo, SEMI_COLON) ? GetNextToken(compiler, &parseInfo->next_token) : throw 0;
        TreeNode *nextTree = Stmt(compiler, parseInfo);
        tmpTree->sibling = nextTree;
        tmpTree = nextTree;
    }
    return intialTree;
}


// program -> stmtseq
TreeNode *Program(CompilerInfo *compiler) {

    TreeNode *parseTree = new TreeNode;
    ParseInfo parseInfo;

    GetNextToken(compiler, &parseInfo.next_token);


    // if file is empty or first token is error
    if (parseInfo->next_token.type == ENDFILE || parseInfo->next_token.type == ERROR) {
        compiler->out_file.Out("end of file");
    } else {
        // get statement sequence
        parseTree = stmtSeq(compiler, &parseInfo);

        // when finish all statements
        if (parseInfo->next_token.type != ENDFILE) {
            compiler->out_file.Out("Error - didn't find end of file (EOF)");
            throw 0;
        }
        return parseTree;
    }


}


int main() {

    CompilerInfo compiler("input.txt", "output.txt", "debug.txt");

    cout<<"Start main()\n";

    TreeNode *parseTree = Program(&compiler);
    PrintTree(parseTree);
    fflush(NULL);

    cout<<"End main()\n";

    return 0;
}
