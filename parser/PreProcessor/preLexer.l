%option noyywrap
%option yylineno

%top {
    #include <iostream>
    #include "preAST.h"
    #include "preParser.h"

    void comment(void);
    int readSrc(void);
    void yyerror(TranslationUnit **unit, const char* s);

    extern int yychar;
}

SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			([Ee][+-]?{D}+)
P                       ([Pp][+-]?{D}+)
FS			(f|F|l|L)
IS                      ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

%%
"//".*  {}
"/*"([^\*]|\*[^\/])*"*/"    {}

^[ \t]*"#include" { g_parseState.isPreProcessorLine = true; return INCLUDE; }
^[ \t]*"#include_next" { g_parseState.isPreProcessorLine = true; return INCLUDE_NEXT; }
^[ \t]*"#line" { g_parseState.isPreProcessorLine = true; return LINE; }
^[ \t]*"#define" {
    g_parseState.isPreProcessorLine = true;
    return DEFINE;
}
^[ \t]*"#if" { g_parseState.isPreProcessorLine = true; return IF; }
^[ \t]*"#else" { g_parseState.isPreProcessorLine = true; return ELSE; }
^[ \t]*"#elif" { g_parseState.isPreProcessorLine = true; return ELIF; }
^[ \t]*"#endif" { g_parseState.isPreProcessorLine = true; return ENDIF; }
^[ \t]*"#ifdef" { g_parseState.isPreProcessorLine = true; return IFDEF; }
^[ \t]*"#ifndef" { g_parseState.isPreProcessorLine = true; return IFNDEF; }
^[ \t]*"#undef" { g_parseState.isPreProcessorLine = true; return UNDEF; }
^[ \t]*"#pragma" { g_parseState.isPreProcessorLine = true; return PRAGMA; }
^[ \t]*"#warning" { g_parseState.isPreProcessorLine = true; return WARNING; }
^[ \t]*"#error" { g_parseState.isPreProcessorLine = true; return ERROR; }

"#" { return PP_ONE; }
"##" { return PP_TWO; }

(\"([^"\\\n]|{ES})*\")	{
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    } else {
        yylval.str = strdup(yytext);
        return STRING_LITERAL;
    }
}

(<([^"\\\n]|{ES})*>)	{ 
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    } else {
        yylval.str = strdup(yytext);
        return LTGT_LITERAL;
    }
}

{L}({L}|{D})* {
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    } else {
        yylval.str = strdup(yytext);
        return SYMBOL;
    }
}

"\"" { 
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    } else {
        return DOUBLE_QUOTE; 
    }
}

"<" { 
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    } else {
        return LT; 
    }
}

">" { 
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    } else {
        return GT; 
    }
}

"\n"   {
    handleLineNumber();
}

. {
    if (!g_parseState.isPreProcessorLine) {
        printf("`%s", yytext);
        src += yytext;
    }
  }
%%

int getYYInput() {
    return yyinput();
}

int handleLineNumber() {
    if (!g_parseState.isPreProcessorLine) {
        src += "\n";
        g_lineNumber++;
    }
    g_parseState.isPreProcessorLine = false;
    printf("\n");
}
