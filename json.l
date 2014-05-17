%{
#include "json.h"
#include "json.tab.hxx"
#include <map>
#include <string>
#include <vector>
#include <utility>

%}

%%

[:,;}{\[\]]  { return yytext[0];}
[0-9]+      {//leading '0' has not yet handled, now
                yylval.i = strtol(yytext,nullptr,10);
                return I;
                }

[0-9]*\.[0-9]*  {
                    if(YYLeng() == 1){
                    }
                    yylval.d = strtold(yytext, nullptr);
                    return F;
                }
\"(\\.|[^"]*)*\"    {
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

