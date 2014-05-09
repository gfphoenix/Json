%{
#include "json.h"
#include "json.tab.hxx"
#include <map>
#include <string>
#include <vector>

%}

%%

[:,;}{\[\]]  {
   // printf("sym <%c>\n", yytext[0]);
    return yytext[0];}
[0-9]+      {
   // printf("number <%s>\n", yytext);
    yylval.i = strtol(yytext,nullptr,10); return I;}
\"(\\.|[^"]*)*\"    {
    //printf("string: <\"%s\">\n", yytext);
                    //strcpy(yylval.s, yytext);
                    yytext++;
                    int n = strlen(yytext);
                    yylval.s = new char[n];
                    memcpy(yylval.s, yytext, n);
                    yylval.s[n-1] = '\0';
                    return S;}
"true"  {yylval.b = true; return B;}
"false"  {yylval.b = false; return B;}

[ \r\t\n]+      {}
.               {printf("ERROR<%s>\n", yytext);}

%%


#if 0
int yyFlexLexer::yywrap(){return 1;}
int main()
{
    yyFlexLexer lexer;
    lexer.yylex();
    return 0;
}
#endif

