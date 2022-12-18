//
// Created by Mariam Khaled on 12/18/2022.
//


#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "syntaxTree.cpp"
using namespace std;


////////////////////////////////////////////////////////////////////////////////////
// Analyzer ////////////////////////////////////////////////////////////////////////
const int SYMBOL_HASH_SIZE=10007;


struct LineLocation {
    int line_num;
    LineLocation *next;
};

struct VariableInfo {
    char *name;
    int memloc;
    LineLocation *head_line; // the head of linked list of source line locations
    LineLocation *tail_line; // the tail of linked list of source line locations
    VariableInfo *next_var; // the next variable in the linked list in the same hash bucket of the symbol table
};

struct SymbolTable {
public:

    int num_vars;
    VariableInfo *var_info[SYMBOL_HASH_SIZE];

    SymbolTable() {
        num_vars = 0;
        int i;
        for (i = 0; i < SYMBOL_HASH_SIZE; i++) var_info[i] = 0;
    }

    int Hash(const char *name) {
        int i, len = strlen(name);
        int hash_val = 11;
        for (i = 0; i < len; i++) hash_val = (hash_val * 17 + (int) name[i]) % SYMBOL_HASH_SIZE;
        return hash_val;
    }

    VariableInfo *Find(const char *name) {
        int h = Hash(name);
        VariableInfo *cur = var_info[h];
        while (cur) {
            if (Equals(name, cur->name)) return cur;
            cur = cur->next_var;
        }
        return 0;
    }

    void Insert(const char *name, int line_num) {
        LineLocation *lineloc = new LineLocation;
        lineloc->line_num = line_num;
        lineloc->next = 0;

        int h = Hash(name);
        VariableInfo *prev = 0;
        VariableInfo *cur = var_info[h];

        while (cur) {
            if (Equals(name, cur->name)) {
                // just add this line location to the list of line locations of the existing var
                cur->tail_line->next = lineloc;
                cur->tail_line = lineloc;
                return;
            }
            prev = cur;
            cur = cur->next_var;
        }

        VariableInfo *vi = new VariableInfo;
        vi->head_line = vi->tail_line = lineloc;
        vi->next_var = 0;
        vi->memloc = num_vars++;
        AllocateAndCopy(&vi->name, name);

        if (!prev) var_info[h] = vi;
        else prev->next_var = vi;
    }

    void Print() {
        int i;
        for (i = 0; i < SYMBOL_HASH_SIZE; i++) {
            VariableInfo *curv = var_info[i];
            while (curv) {
                printf("[Var=%s][Mem=%d]", curv->name, curv->memloc);
                LineLocation *curl = curv->head_line;
                while (curl) {
                    printf("[Line=%d]", curl->line_num);
                    curl = curl->next;
                }
                printf("\n");
                curv = curv->next_var;
            }
        }
    }

    void Destroy() {
        int i;
        for (i = 0; i < SYMBOL_HASH_SIZE; i++) {
            VariableInfo *curv = var_info[i];
            while (curv) {
                LineLocation *curl = curv->head_line;
                while (curl) {
                    LineLocation *pl = curl;
                    curl = curl->next;
                    delete pl;
                }
                VariableInfo *p = curv;
                curv = curv->next_var;
                delete p;
            }
            var_info[i] = 0;
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///// Running the program's simulation

// fast-power function, is a function that calculates the power
// of a number but in complexity: O(log n) => faster than normal power function
int fastPower( int a , int b )
{
    if(b==0)
        return 1;

    int tmp = fastPower(a, b/2);
    tmp = tmp * tmp;

    if(b&1)
        tmp = tmp * a;

    return tmp;

}


// this function is used for evaluating the expressions
// to make the internal process of the program
int solve(TreeNode* node,SymbolTable* symbolTable, int *variablesArray)
{

    // if number or id
    // and other operands other than (<= , =)
    // +, -, *,  /, ^
    if(node->node_kind == NUM_NODE )
        return node->num;
    if(node->node_kind == ID_NODE )
        return variablesArray[symbolTable->Find( node->id )->memloc];

    // get the evaluation of the operands/expressions
    // around the operator
    int first = solve(node->child[0], symbolTable, variablesArray);
    int second = solve(node->child[1], symbolTable, variablesArray);

    // otherwise they are "operators" or "error"
    if(node->oper == EQUAL)
    {
        return (first == second);
    }else if(node->oper == LESS_THAN)
    {
        return (first<=second);
    }else if( node->oper == PLUS )
    {
        return first+second;
    }else if(node->oper == MINUS)
    {
        return first-second;
    }else if (node->oper == TIMES)
    {
        return first*second;
    }else if (node->oper == DIVIDE)
    {
        return first/second;
    }else if (node->oper == POWER)
    {
        return fastPower(first, second);
    }else
    {
        string errorMsg = "Error: Invalid Mathematical or Boolean expression's syntax\n";
        throw errorMsg;
    }


}


// runs the program => executes the program
void RunProgram(TreeNode* currentNode, SymbolTable* symbolTable, int *variablesArray)
{

    // checking on the node's type to assign a function for it
    if( currentNode->node_kind == READ_NODE )
    {
        printf("Please Enter the value you want: ");
        int value;
        scanf("%d \n", &value);

        // to save the entered value from user to its memory location in the symbol table
        // => real program have memory locations (this saves it in its memory location)
        variablesArray[symbolTable->Find( currentNode->id )->memloc];

    }else if ( currentNode->node_kind == WRITE_NODE )
    {
        // prints the value to the terminal
        int value = solve(currentNode->child[0], symbolTable, variablesArray);
        printf("The value is: %d\n", value);

    }else if ( currentNode->node_kind == ASSIGN_NODE )
    {
        // assigns the value of this node to its memory location
        // the value; would be returned from the solve function
        int value = solve(currentNode->child[0], symbolTable, variablesArray);
        variablesArray[symbolTable->Find( currentNode->id )->memloc] = value;

    }else if ( currentNode->node_kind == IF_NODE )
    {
        // check if this condition is true
        int conditionChecker = solve(&currentNode[0], symbolTable, variablesArray);

        if(conditionChecker)
        {
            // if the condition is true=> run the part after "then"
            RunProgram(currentNode->child[1], symbolTable, variablesArray);
        }else if (currentNode->child[2])
        {
            // if the condition is false=> run the part after "else"
            // as long as there is "else" block
            RunProgram(currentNode->child[2], symbolTable, variablesArray);
        }


    }else if ( currentNode->node_kind == REPEAT_NODE )
    {
        //// todo : complete it
        do{

            // evaluate the expression
            solve(currentNode->child[0], symbolTable, variablesArray);

        }while( !solve(currentNode->child[1], symbolTable, variablesArray) );

    }

    // to check if this node-> have siblings in the same level
    if( currentNode->sibling != NULL )
        RunProgram( currentNode->sibling, symbolTable, variablesArray );


}


// function that runs the simulation of the program
void RunSimulation(TreeNode* syntaxTree, SymbolTable* symbolTable)
{
    // initializing the variables with 0
    int variablesNumbers = symbolTable->num_vars;
    int* variablesArray = new int [variablesNumbers];
    for(int i = 0; i<variablesNumbers; ++i)
    {
        variablesArray[i] = 0;
    }

    // calling the actual program running
    RunProgram(syntaxTree, symbolTable, variablesArray);

    // emptying the array
    delete[] variablesArray;

}

