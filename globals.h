/****************************************************/
/* File: globals.h                                  */
/* Global types and vars for TINY compiler          */
/* must come before other include files             */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 6


typedef enum 
{ 
    /* reserved words */
    IF,ELSE,INT,RETURN,VOID,WHILE,
    /* special symbols */
    PLUS,MINUS,TIMES,DIVIDE,LT,GT,ASSIGN,NEQ,SEMI,COMMA,LPAREN,RPAREN,LBRACE,RBRACE,LSQUARE,RSQUARE,LTE,GTE,EQ,
    /* multicharacter tokens */
    NUM,ID,
    /* book-keeping tokens */
    ENDOFFILE,ERROR,
   } TokenType;

FILE* source; /* source code text file */
FILE* listing; /* listing output text file */
FILE* code; /* code text file for TM simulator */

extern int lineno; /* source line number for listing */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

typedef enum {StmtK,ExpK,DecK} NodeKind;
typedef enum {IfK,WhileK,ReturnK,CallK,CompoundK} StmtKind;
typedef enum {OpK,ConstK,IdK,AssignK} ExpKind;
typedef enum { ScalarDecK,FuncDecK,ArrayDecK } DecKind;

/* ExpType is used for type checking */
typedef enum {Void,Integer,Function,Array} ExpType;

#define MAXCHILDREN 3

typedef struct treeNode
   { struct treeNode * child[MAXCHILDREN];
     struct treeNode * sibling;
     int lineno;
     NodeKind nodekind;
     union { StmtKind stmt; ExpKind exp; DecKind dec; } kind;
     TokenType op;
     int val;
     char * name; 
     ExpType functionReturnType;
     ExpType variableDataType;
     ExpType expressionType;
     int isParameter;
     struct treeNode* declaration;
     /* for type checking of exps */
   } TreeNode;
   
typedef struct HS {
    struct HS* next;
    TreeNode* declaration;
    char* name;
    int symbleAlreadyDeclared;
    int lineFirstReferenced;
}HashNode;

typedef HashNode* HashNodePtr;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern _Bool Error;
#endif
