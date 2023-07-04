%option noyywrap
%option yylineno

%top {
    #include <iostream>
    #include "preAST.h"
    #include "preParser.h"

    void comment(void);
    void yyerror(TranslationUnit **unit, const char* s);
}

SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]

%%
"/*" { comment(); }
"//"[^\n]* { /* consume //-comment */ }
"#include" { return INCLUDE; }
"#include_next" { return INCLUDE_NEXT; }
"#" { return PP_ONE; }
"##" { return PP_TWO; }
"#line" { return LINE; }
"#define" { return DEFINE; }
"#if" { return IF; }
"#else" { return ELSE; }
"#elif" { return ELIF; }
"#endif" { return ENDIF; }
"#ifdef" { return IFDEF; }
"#ifndef" { return IFNDEF; }
"#undef" { return UNDEF; }
"#pragma" { return PRAGMA; }
"#warning" { return WARNING; }
"#error" { return ERROR; }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	{ 
    yylval.str = strdup(yytext);
    return STRING_LITERAL;
}

({SP}?<([^"\\\n]|{ES})*>{WS}*)+	{ 
    yylval.str = strdup(yytext);
    return LTGT_LITERAL;
}

"\"" { return DOUBLE_QUOTE; }
"<" { return LT; }
">" { return GT; }
"\r"    {}
"\n"    {}

. ;

%%

void comment(void) {
  char c, prev = 0;

  while ((c = yyinput()) != 0) /* (EOF maps to 0) */
  {
    if (c == '/' && prev == '*')
      return;
    prev = c;
  }
  yyerror(NULL, "unterminated comment");
}