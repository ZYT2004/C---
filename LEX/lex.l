%{
/* - coding: ASCII - */
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN + 1];
void printToken(TokenType token, const char* tokenString);
%}

digit	[0-9]
number	{digit}+
letter	[a-zA-Z]
identifier {letter}+
newline     \n
whitespace    [ \t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"return"        {return RETURN;}
"int"           {return INT;}
"void"          {return VOID;}
"while"		{return WHILE;}
"="             {return ASSIGN;}
"=="            {return EQ;}
"<"             {return LT;}
"<="            {return LTE;}
">"             {return GT;}
">="            {return GTE;}
"!="            {return NEQ;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return DIVIDE;}
"("             {return LPAREN;}
")"             {return RPAREN;}
";"             {return SEMI;}
","		{return COMMA;}
"}"             {return RBRACE;}
"{"		{return LBRACE;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
"/*"            {
    		char c;
    		int flag = 1;
    		do
    		{
        	c = input();
        	if (c == EOF) break;
        	if (c == '\n') lineno++;
        	if (c == '*')
        	{
            	c = input();
            	if (c == '/') flag = 0;
        	}
    		} while (flag);
		}
.		{return ERROR;}
<<EOF>>		{return ENDOFFILE;}

    
%%

TokenType getToken(void)
{
    static int firstTime = TRUE;
    TokenType currentToken;
    if (firstTime)
    {
        firstTime = FALSE;
        lineno++;
        yyin = source;
        yyout = listing;
    }
    currentToken = yylex();
    strncpy(tokenString, yytext, MAXTOKENLEN);
    if (TraceScan) {
        fprintf(listing, "\t%d: ", lineno);
        printToken(currentToken, tokenString);
    }
    return currentToken;
}
int yywrap()
{
    return 0;
}

