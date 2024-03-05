/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include"globals.h"
#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#define MAXTABLESIZE 233
#define HIGHWATERMARK "__invalid__"


/* The hashtable itself */
static HashNodePtr hashtable[MAXTABLESIZE];
 
char* typeName(ExpType e);

/* The "temporary list", used to track scopes. */
static HashNodePtr tempList;

int scopeDepth;

static HashNodePtr allocateSymbolNode(char* name,TreeNode* declaration,int lineDefined);

/* hashfunction(): takes a string and generates a hash value. */
static int hashFunction(char* key);

/* error reporting */
static void flagError(char* message);

/* used in symbol table scope dump */
static char* formatSymbolType(TreeNode* node);

/* the guts of dumpCurrentScope() */
static void startDumpCurrentScope(HashNodePtr cursor);


/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * name, int lineno, int loc );

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * name );

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

void insertSymbol(char* name, TreeNode* symbolDefNode, int lineDefined);

void newScope();

void dumpCurrentScope();

HashNodePtr lookupSymbol(char* name);

void endScope();

#endif
