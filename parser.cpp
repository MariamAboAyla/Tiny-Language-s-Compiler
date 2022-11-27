#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std;


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

    TreeNode *node = new TreeNode;
    node->line_num = compiler->in_file.cur_line_num;

    if (checkOnType(compiler, parseInfo, LEFT_PAREN)) {
        // to get the next command
        GetNextToken(compiler, &parseInfo->next_token);
        node = mathExpression(compiler, parseInfo);

        // get next token: to check if it will take right bracket or error
        if (!checkOnType(compiler, parseInfo, RIGHT_PAREN))
            throw 0;

        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);

    } else if (checkOnType(compiler, parseInfo, ID)) {
        node->node_kind = ID_NODE;

        // to add the value of the ID to the node
        AllocateAndCopy(&node->id, parseInfo->next_token.str);

        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);

    } else if (checkOnType(compiler, parseInfo, NUM)) {
        node->node_kind = NUM_NODE;

        // to add the value of the num to the node
        node->num = stoi(parseInfo->next_token.str);

        // to get the next comment
        GetNextToken(compiler, &parseInfo->next_token);

    } else {
        throw 0;
    }
    return node;
}

void copyTreeNode(TreeNode* Node1,TreeNode* Node2){
    for (int i = 0; i < 3; ++i) {
        Node1->child[i] = Node2->child[i];
    }
    Node1->node_kind = Node2->node_kind;
    Node1->line_num = Node2->line_num;
    Node1->sibling = Node2->sibling;
    Node1->oper = Node2->oper;
    Node1->id = Node2->id;
    Node1->num = Node2->num;
    Node1->expr_data_type =Node2->expr_data_type;

}

// Factor function
// factor -> newexpr { ^ newexpr }    right associative
TreeNode *Factor(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *leftNode = newExpression(compiler, parseInfo);
    TreeNode *node = new TreeNode;
    TreeNode *rootNode = new TreeNode;
    copyTreeNode(node,leftNode);
    copyTreeNode(rootNode,leftNode);
    int count=-1;

    bool isFirst = true;

    while (parseInfo->next_token.type == POWER) {
        TreeNode *newSubtree = new TreeNode();
        newSubtree->node_kind = OPER_NODE;
        newSubtree->line_num = compiler->in_file.cur_line_num;
        newSubtree->oper = parseInfo->next_token.type;

        // assign the left-node to the OPER node
        newSubtree->child[0] = new TreeNode();
        copyTreeNode(newSubtree->child[0],node);

        // get the next token -> right node of the OPER_NODE
        GetNextToken(compiler, &parseInfo->next_token);

        // assign the right-node to the OPER node
        newSubtree->child[1] = new TreeNode();
        TreeNode *rightNode = newExpression(compiler, parseInfo);
        copyTreeNode(newSubtree->child[1],rightNode);


        count++;
        copyTreeNode(node,rightNode); //5
        if(isFirst)
        {
            copyTreeNode(rootNode,newSubtree);
            isFirst = false;
        }else{
            TreeNode* tmpNode = rootNode;
            for (int i = 0; i < count; ++i) {
                tmpNode = tmpNode->child[1];
            }
            copyTreeNode(tmpNode,newSubtree);
        }
    }
    return rootNode;
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


    // Else part - [Optional]
    if (checkOnType(compiler, parseInfo, ELSE)) {
        GetNextToken(compiler, &parseInfo->next_token);
        node->child[2] = stmtSeq(compiler, parseInfo);
    }

    // End of If Statement
    if (!checkOnType(compiler, parseInfo, END)) throw 0;

    GetNextToken(compiler, &parseInfo->next_token);
    return node;
}

// repeatstmt -> repeat stmtseq until expr
TreeNode *repeatStmt(CompilerInfo *compiler, ParseInfo *parseInfo) {
    TreeNode *node = new TreeNode;
    node->line_num = compiler->in_file.cur_line_num;
    node->node_kind = REPEAT_NODE;
    GetNextToken(compiler, &parseInfo->next_token);
    node->child[0] = stmtSeq(compiler, parseInfo);

//    GetNextToken(compiler, &parseInfo->next_token);

    //check whether it is "until"
    checkOnType(compiler, parseInfo, UNTIL) ? GetNextToken(compiler, &parseInfo->next_token) : throw 0;

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

        if (!checkOnType(compiler, parseInfo, SEMI_COLON)) throw 0;

        GetNextToken(compiler, &parseInfo->next_token);

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
    if (parseInfo.next_token.type == ENDFILE || parseInfo.next_token.type == ERROR) {
        compiler->out_file.Out("end of file");
    } else {
        // get statement sequence
        parseTree = stmtSeq(compiler, &parseInfo);

        // when finish all statements
        if (parseInfo.next_token.type != ENDFILE) {
            compiler->out_file.Out("Error - didn't find end of file (EOF)");
            throw 0;
        }
        return parseTree;
    }


}
