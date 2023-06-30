%parse-param {int *v}

%code top {
    #include "preParser.h"
    #include "stdio.h"

    extern int yylex(void);
    extern int yylineno;

    static void yyerror(int *v, const char* s) {
        fprintf(stderr, "Parse Error In Line %d\n", yylineno);
    }
}

%token PP_ONE PP_TWO INCLUDE INCLUDE_NEXT LINE DEFINE IF ELSE ELIF ENDIF IFDEF IFNDEF UNDEF PRAGMA WARNING ERROR

// Grammar rules

%%

start : {}
;

%%