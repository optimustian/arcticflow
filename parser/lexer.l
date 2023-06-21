%option noyywrap
%option yylineno

%top {
    #include "SimAST.h"
    #include "sim_parser.h"
    #include <iostream>
}

D   [0-9]
L   [a-zA-Z_]
NZ  [1-9]

%%

"hardware"  { return HARDWARE; }
"operator"  { return OPERATOR; }
"simu"      { return SIMU; }
"foreach"   { return FOREACH; }
"sleep"     { return SLEEP; }
"="         { return ASSIGN; }
"{"         { return LEFT_BIG_PAR; }
"}"         { return RIGHT_BIG_PAR; }
"["         { return LEFT_MID_PAR; }
"]"         { return RIGHT_MID_PAR; }
"("         { return LEFT_SMALL_PAR; }
")"         { return RIGHT_SMALL_PAR; }
","         { return COMMA; }
"."     { return DOT; }
"->"    { return ARROW; }
";"     { return SEMI;}
"#".*   {};
"\r"    {};
"\n"    {};

{L}({L}|{D})* {
    // TODO(tian) : may have memory leak
    yylval.str = strdup(yytext);
    return SYMBOL;
}

{NZ}{D}* {
    yylval.iVal = atoi(yytext);
    return I_CONSTANT;
}

{D}*"."{D} {
    yylval.fVal = atof(yytext);
    return F_CONSTANT;
}

. ;

%%