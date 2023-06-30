%option noyywrap
%option yylineno

%top {
    #include <iostream>
    #include "preParser.h"
}

%%

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
"\r"    {}
"\n"    {}

. ;

%%