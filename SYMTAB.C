#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include<assert.h>

#include "Globals.h"
#include "SymTab.h"
#include "Util.h"
#include"PARSE.H"

#define MAXTABLESIZE 233
#define HIGHWATERMARK "__invalid__"

_Bool Error;

static HashNodePtr allocateSymbolNode(char* name,
    TreeNode* declaration,
    int lineDefined);

/* hashfunction(): takes a string and generates a hash value. */
static int hashFunction(char* key);

/* error reporting */
static void flagError(char* message);

/* used in symbol table scope dump */
static char* formatSymbolType(TreeNode* node);

/* the guts of dumpCurrentScope() */
static void startDumpCurrentScope(HashNodePtr cursor);

void initSymbolTable(void)
{
    
	memset(hashtable, 0, (sizeof(HashNodePtr) )* MAXTABLESIZE);
    tempList = NULL;
}

/* Check to see if the symbol given by "name" is already declared in thecurrent scope. */

int symbolAlreadyDeclared(char* name)
{
    int symbolFound = FALSE;
    HashNodePtr cursor;

    /* Scan "tempList" within _current_ scope for duplicate definition */
    cursor = tempList;

    while ((cursor != NULL) && (!symbolFound) && ((strcmp(cursor->name, HIGHWATERMARK) != 0)))
    {
        if (strcmp(name, cursor->name) == 0)
            symbolFound = TRUE;
        else
            cursor = cursor->next;
    }

    return (symbolFound);
}

static HashNodePtr allocateSymbolNode(char* name,
    TreeNode* declaration,
    int lineDefined)
{
    HashNodePtr temp;
    temp = (HashNode*)malloc(sizeof(HashNode));
    if (temp == NULL)
    {
        Error = TRUE;
        fprintf(listing,
            "*** Out of memory allocating memory for symbol table\n");
    }
    else
    {
        temp->name = copyString(name);
        temp->declaration = declaration;
        temp->lineFirstReferenced = lineDefined;
        temp->next = NULL;
    }

    return temp;
}

void insertSymbol(char* name, TreeNode* symbolDefNode, int lineDefined)
{
    char errorString[80];

    HashNodePtr newHashNode, temp;
    int hashBucket;

    /* If the symbol already exists, flag an error */
    if (symbolAlreadyDeclared(name))
    {
        sprintf(errorString, "duplicate identifier \"%s\"\n", name);
        flagError(errorString);
    }
    else
    {
        /* Locate bucket we're using */
        hashBucket = hashFunction(name);
        /* Allocate and insert record on front of bucket */
        newHashNode = allocateSymbolNode(name, symbolDefNode, lineDefined);
        if (newHashNode != NULL)
        {
            temp = hashtable[hashBucket];
            hashtable[hashBucket] = newHashNode;
            newHashNode->next = temp;
        }

        /* Stick node on front of "tempList" */
        newHashNode = allocateSymbolNode(name, symbolDefNode, lineDefined);
        if (newHashNode != NULL)
        {
            temp = tempList;
            tempList = newHashNode;
            tempList->next = temp;
        }
    }
}


HashNodePtr lookupSymbol(char* name)
{
    HashNodePtr cursor;
    int hashBucket;    /* hash bucket on which to conduct our search */
    int found = FALSE;

    hashBucket = hashFunction(name);
    cursor = hashtable[hashBucket];

    while (cursor != NULL)
    {
        if (strcmp(name, cursor->name) == 0)
        {
            found = TRUE;
            break;
        }

        cursor = cursor->next;
    }

    if (found == TRUE)
        return cursor;
    else
        return NULL;
}

/*
 * Use recursion to dump symbols out in "reverse-reverse" order.. i.e the
 *  right way around...
 */

void dumpCurrentScope()
{
    HashNodePtr cursor;

    cursor = tempList;

    /* if the current scope isn't empty,  dump it out */
    if ((cursor != NULL) && (strcmp(HIGHWATERMARK, cursor->name)))
        startDumpCurrentScope(cursor);
}

#define IDENT_LEN 12

static void startDumpCurrentScope(HashNodePtr cursor)
{
    char paddedIdentifier[IDENT_LEN + 1];
    char* typeInformation; /* used to catch result of formatSymbolType */

    if ((cursor->next != NULL) && (strcmp(cursor->next->name, HIGHWATERMARK) != 0))
        startDumpCurrentScope(cursor->next);

    /* pad identifier name */
    memset(paddedIdentifier, ' ', IDENT_LEN);
    memmove(paddedIdentifier, cursor->name, strlen(cursor->name));
    paddedIdentifier[IDENT_LEN] = '\0';

    /* output symbol table entry */
    typeInformation = formatSymbolType(cursor->declaration);

    fprintf(listing, "%3d   %s   %7d     %c    %s\n",
        scopeDepth,
        paddedIdentifier,
        cursor->lineFirstReferenced,
        cursor->declaration->isParameter ? 'Y' : 'N',
        typeInformation);

    /* Prevent a memory leak - (bjf, 11/5/2000) */
    free(typeInformation);
}

void newScope()
{
    HashNodePtr newNode, temp;

    newNode = allocateSymbolNode(HIGHWATERMARK, NULL, 0);
    if (newNode != NULL)
    {
        temp = tempList;
        tempList = newNode;
        tempList->next = temp;
    }
}

void endScope()
{
    /*
     *  endScope()'s job is to delete all symbols in the current scope.  It
     *  works by scanning "tempList" from the front and for each record
     *  before the high-water mark, it deletes each symbol's occurrence in
     *  the hash table.  This is done until the high-water mark is consumed.
     */

    HashNodePtr hashPtr;
    HashNodePtr temp; /* used in freeing HashNodes */
    int hashBucket;

    while ((tempList != NULL) && (strcmp(HIGHWATERMARK, tempList->name)) != 0)
    {
        /* locate this node in the hash table, delete it */
        hashBucket = hashFunction(tempList->name);
        hashPtr = hashtable[hashBucket];

        /*
         *  INVARIANT: since symbols were inserted into the hash table _and_
         *    tempList in the same order, the name of the symbol on the
         *    front of the hash bucket _must_ be the same as the one under
         *    tempListPtr.
         */

        assert((tempList != NULL) && (hashtable[hashBucket] != NULL));
        assert(strcmp(tempList->name, hashPtr->name) == 0);

        /* delete from hash table */
        temp = hashtable[hashBucket]->next;
        free(hashtable[hashBucket]);
        hashtable[hashBucket] = temp;

        /* ... and from second list */
        temp = tempList->next;
        free(tempList);
        tempList = temp;
    }

    /* delete high water mark */
    assert(strcmp(tempList->name, HIGHWATERMARK) == 0);
    temp = tempList->next;
    free(tempList);
    tempList = temp;
}


/* Power-of-two multiplier in hash function */
#define SHIFT 4

/* Code borrowed from Louden p.522 */
static int hashFunction(char* key)
{
    int temp = 0;
    int i = 0;

    while (key[i] != '\0')
    {
        temp = ((temp << SHIFT) + key[i]) % MAXTABLESIZE;
        ++i;
    }

    return temp;
}

static void flagError(char* message)
{
    fprintf(listing, ">>> Semantic error (symbol table): %s", message);
    Error = TRUE; /* global variable to inhibit subseq. passes on error */
}

static char* formatSymbolType(TreeNode* node)
{
    char stringBuffer[100];

    if ((node == NULL) || (node->nodekind != DecK))
        strcpy(stringBuffer, "<<ERROR>>");
    else
    {
        /* node is a declaration */
        switch (node->kind.dec)
        {
        case ScalarDecK:
            sprintf(stringBuffer, "Scalar of type %s",
                typeName(node->variableDataType));
            break;
        case ArrayDecK:
            sprintf(stringBuffer, "Array of type %s with %d elements",
                typeName(node->variableDataType), node->val);
            break;
        case FuncDecK:
            sprintf(stringBuffer, "Function with return type %s",
                typeName(node->functionReturnType));
            break;
        default:
            strcpy(stringBuffer, "<<UNKNOWN>>");
            break;
        }
    }

    return copyString(stringBuffer);
}
