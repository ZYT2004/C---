#define _CRT_SECURE_NO_WARNINGS

#include "Analyze.h"
#include "Globals.h"
#include "SymTab.h"
#include "Util.h"
#include"parse.h"
/* draw a ruler on the screen */
static void drawRuler(FILE* output, char* string);

/* the guts of buildSymbolTable() */
static void startBuildSymbolTable(TreeNode* syntaxTree);

/* flag an error from the type checker */
static void flagSemanticError(char* str);

/* generic tree traversal routine */
static void traverse(TreeNode* syntaxTree,
    void (*preProc)(TreeNode*),
    void (*postProc)(TreeNode*));

/* routine to perform the actual type check on a node */
static void checkNode(TreeNode* syntaxTree);

/* dummy do-nothing procedure used to keep traversal() happy */
static void nullProc(TreeNode* syntaxTree);

/* traverse the syntax tree, marking global variables as such */
void markGlobals(TreeNode* tree);

/* declare the C-minus "built-in" input() and output() routines */
static void declarePredefines(void);

/* type-check functions' formal parameters against actual parameters */
static int checkFormalAgainstActualParms(TreeNode* formal, TreeNode* actual);

int TraceAnalyze = 1;

_Bool Error;

void buildSymTab(TreeNode* syntaxTree)
{
    /* Format headings */
    if (TraceAnalyze)
    {
		drawRuler(listing, "");
        fprintf(listing,"Scope Identifier  Line  Is a  Symbol type\n");
        fprintf(listing,"depth  Decl. parm?\n");
    }

    declarePredefines(); /* make input() and output() visible in globals */
    startBuildSymbolTable(syntaxTree);
}

void typeCheck(TreeNode* syntaxTree)
{
    traverse(syntaxTree, nullProc, checkNode);
}

/* make input() and output() visible in globals */
static void declarePredefines(void)
{
    TreeNode* input;
    TreeNode* output;
    TreeNode* temp;

    /* define "int input(void)" */
    input = newDecNode(FuncDecK);
    input->name = copyString("input");
    input->functionReturnType = Integer;
    input->expressionType = Function;

   
    temp = newDecNode(ScalarDecK);
    temp->name = copyString("arg");
    temp->variableDataType = Integer;
    temp->expressionType = Integer;
    
	/* define "void output(int)" */
    output = newDecNode(FuncDecK);
    output->name = copyString("output");
    output->functionReturnType = Void;
    output->expressionType = Function;
    output->child[0] = temp;

    /* get input() and output() added to global scope */
    insertSymbol("input", input, 0);
    insertSymbol("output", output, 0);
}

static void startBuildSymbolTable(TreeNode* syntaxTree)
{
    int i;                     /* iterate over node children */
    HashNodePtr currentSymbol; /* symbol being looked up */
    char errorMessage[80];

    /* used to decorate RETURN nodes with enclosing procedure */
    static TreeNode* enclosingFunction = NULL;

    while (syntaxTree != NULL)
    {
        /* Examine current symbol: if it's a declaration, insert intosymbol table. */
        if (syntaxTree->nodekind == DecK)
            insertSymbol(syntaxTree->name, syntaxTree, syntaxTree->lineno);

        /* If entering a new function, tell the symbol table */
        if ((syntaxTree->nodekind == DecK) && (syntaxTree->kind.dec == FuncDecK))
        {
            /* record the enclosing procedure declaration */
            enclosingFunction = syntaxTree;

            if (TraceAnalyze)
                drawRuler(listing, syntaxTree->name);

            newScope();
            ++scopeDepth;
        }

        /* if entering a compound-statement, create a new scope as well */
        if ((syntaxTree->nodekind == StmtK) && (syntaxTree->kind.stmt == CompoundK))
        {
            newScope();
            ++scopeDepth;
        }

        /* if it's an identifier, it needs to be check symbol table*/
        if (((syntaxTree->nodekind == ExpK) && (syntaxTree->kind.exp == IdK))
            || ((syntaxTree->nodekind == StmtK) && (syntaxTree->kind.stmt == CallK)))
        {
            currentSymbol = lookupSymbol(syntaxTree->name);
            if (currentSymbol == NULL)
            {
                /* operation failed; say so to user */
                sprintf(errorMessage,
                    "identifier \"%s\" unknown or out of scope\n",
                    syntaxTree->name);
                flagSemanticError(errorMessage);
            }
            else
                syntaxTree->declaration = currentSymbol->declaration;
        }

        /* mark return type */
        if ((syntaxTree->nodekind == StmtK) &&
            (syntaxTree->kind.stmt == ReturnK))
        {
            syntaxTree->declaration = enclosingFunction;
        }

        for (i = 0; i < MAXCHILDREN; ++i)
            startBuildSymbolTable(syntaxTree->child[i]);

        /* If leaving a scope, tell the symbol table */
        if (((syntaxTree->nodekind == DecK) && (syntaxTree->kind.dec == FuncDecK))
            || ((syntaxTree->nodekind == StmtK) && (syntaxTree->kind.stmt == CompoundK)))
        {
            if (TraceAnalyze)
                dumpCurrentScope();
            --scopeDepth;
            endScope();
        }
        syntaxTree = syntaxTree->sibling;
    }
}

static void drawRuler(FILE* output, char* string)
{
    int length;
    int numTrailingDashes;
    int i;

    /* empty string */
    if (strcmp(string, "") == 0)
        length = 0;
    else
        length = strlen(string) + 2;

    fprintf(output, "---");
    if (length > 0)
        fprintf(output, " %s ", string);
    numTrailingDashes = 45 - length;

    for (i = 0; i < numTrailingDashes; ++i)
        fprintf(output, "-");
    fprintf(output, "\n");
}

static void flagSemanticError(char* str)
{
    fprintf(listing, ">>> Semantic error (type checker): %s", str);
    Error = TRUE;
}

/* generic tree traversal routine */
static void traverse(TreeNode* syntaxTree,
    void (*preProc)(TreeNode*),
    void (*postProc)(TreeNode*))
{
    while (syntaxTree != NULL)
    {
        preProc(syntaxTree);
        for (int i = 0; i < MAXCHILDREN; ++i)
            traverse(syntaxTree->child[i], preProc, postProc);
        postProc(syntaxTree);
        syntaxTree = syntaxTree->sibling;
    }
}

static int checkFormalAgainstActualParms(TreeNode* formal, TreeNode* actual)
{
    TreeNode* firstList;
    TreeNode* secondList;

    firstList = formal->child[0];
    secondList = actual->child[0];

    while ((firstList != NULL) && (secondList != NULL))
    {
        if (firstList->expressionType != secondList->expressionType)
            return FALSE;

        if (firstList)
            firstList = firstList->sibling;
        if (secondList)
            secondList = secondList->sibling;
    }

    if (((firstList == NULL) && (secondList != NULL))
        || ((firstList != NULL) && (secondList == NULL)))
        return FALSE;

    return TRUE;
}

static void checkNode(TreeNode* syntaxTree)
{
    char errorMessage[100];

    switch (syntaxTree->nodekind)
    {
    case DecK:

        switch (syntaxTree->kind.dec)
        {
        case ScalarDecK:
            syntaxTree->expressionType = syntaxTree->variableDataType;
            break;

        case ArrayDecK:
            syntaxTree->expressionType = Array;
            break;

        case FuncDecK:
            syntaxTree->expressionType = Function;
            break;
        }

        break; /* case DecK */

    case StmtK:

        switch (syntaxTree->kind.stmt)
        {
        case IfK:

            if (syntaxTree->child[0]->expressionType != Integer)
            {
                sprintf(errorMessage,
                    "IF-expression must be integer (line %d)\n",
                    syntaxTree->lineno);
                flagSemanticError(errorMessage);
            }
            break;

        case WhileK:

            if (syntaxTree->child[0]->expressionType != Integer)
            {
                sprintf(errorMessage,
                    "WHILE-expression must be integer (line %d)\n",
                    syntaxTree->lineno);
                flagSemanticError(errorMessage);
            }
            break;

        case CallK:

            /*  Check types and numbers of formal against actual parameters */
            if (!checkFormalAgainstActualParms(syntaxTree->declaration,
                syntaxTree))
            {
                sprintf(errorMessage, "formal and actual parameters to "
                    "function don\'t match (line %d)\n",
                    syntaxTree->lineno);
                flagSemanticError(errorMessage);
            }
            syntaxTree->expressionType = syntaxTree->declaration->functionReturnType;
            break;

        case ReturnK:

            /* match return type */
            if (syntaxTree->declaration->functionReturnType == Integer)
            {
                if ((syntaxTree->child[0] == NULL) ||
                    (syntaxTree->child[0]->expressionType != Integer))
                {
                    sprintf(errorMessage, "RETURN-expression is either "
                        "missing or not integer (line %d)\n",
                        syntaxTree->lineno);
                    flagSemanticError(errorMessage);
                }
            }
            else if (syntaxTree->declaration->functionReturnType == Void)
            {
                /* does a return-expression exist? complain */
                if (syntaxTree->child[0] != NULL)
                {
                    sprintf(errorMessage, "RETURN-expression must be"
                        "void (line %d)\n",
                        syntaxTree->lineno);
                }
            }

            break;

        case CompoundK:

            syntaxTree->expressionType = Void;
            break;
        }

        break; /* case StmtK */

    case ExpK:

        switch (syntaxTree->kind.exp)
        {
        case OpK:
            /* Arithmetic operators */
            if ((syntaxTree->op == PLUS) || (syntaxTree->op == MINUS) ||
                (syntaxTree->op == TIMES) || (syntaxTree->op == DIVIDE))
            {
                if ((syntaxTree->child[0]->expressionType == Integer) &&
                    (syntaxTree->child[1]->expressionType == Integer))
                    syntaxTree->expressionType = Integer;
                else
                {
                    sprintf(errorMessage, "arithmetic operators must have "
                        "integer operands (line %d)\n",
                        syntaxTree->lineno);
                    flagSemanticError(errorMessage);
                }
            }
            /* Relational operators */
            else if ((syntaxTree->op == GT) || (syntaxTree->op == LT) ||
                (syntaxTree->op == LTE) || (syntaxTree->op == GTE) ||
                (syntaxTree->op == EQ) || (syntaxTree->op == NEQ))
            {
                if ((syntaxTree->child[0]->expressionType == Integer) &&
                    (syntaxTree->child[1]->expressionType == Integer))
                    syntaxTree->expressionType = Integer;
                else
                {
                    sprintf(errorMessage, "relational operators must have "
                        "integer operands (line %d)\n",
                        syntaxTree->lineno);
                    flagSemanticError(errorMessage);
                }
            }
            else
            {
                sprintf(errorMessage, "error in type checker: unknown operator"
                    " (line %d)\n",
                    syntaxTree->lineno);
                flagSemanticError(errorMessage);
            }

            break;

        case IdK:

            if (syntaxTree->declaration->expressionType == Integer)
            {
                if (syntaxTree->child[0] == NULL)
                    syntaxTree->expressionType = Integer;
                else
                {
                    sprintf(errorMessage, "identifier is an illegal type "
                        "(line %d)\n",
                        syntaxTree->lineno);
                    flagSemanticError(errorMessage);
                }
            }
            break;

        case ConstK:

            syntaxTree->expressionType = Integer;
            break;

        case AssignK:

            /* Variable assignment */
            if ((syntaxTree->child[0]->expressionType == Integer) &&
                (syntaxTree->child[1]->expressionType == Integer))
                syntaxTree->expressionType = Integer;
            else
            {
                sprintf(errorMessage, "both assigning and assigned expression"
                    " must be integer (line %d)\n",
                    syntaxTree->lineno);
                flagSemanticError(errorMessage);
            }

            break;
        }

        break; /* case ExpK */

    } /* switch (syntaxTree->nodekind) */
}

/* dummy do-nothing procedure used to keep traverse() happy */
static void nullProc(TreeNode* syntaxTree)
{
    return;
}
int main(){
    TreeNode* syntaxTree = parse();
  	listing=fopen("D:\\C--±‡“Î∆˜\\listing.txt","w");
	fprintf(listing,"\nBuilding Symbol Table...\n");
    buildSymTab(syntaxTree);
    fprintf(listing,"\nChecking Types...\n");
    typeCheck(syntaxTree);
    fprintf(listing,"\nType Checking Finished\n");
  }
