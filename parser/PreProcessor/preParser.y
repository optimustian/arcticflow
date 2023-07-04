%parse-param {TranslationUnit **unit}

%code top {
    #include "preAST.h"
    #include "preParser.h"
    #include "stdio.h"

    extern int yylex(void);
    extern int yylineno;

    void yyerror(TranslationUnit **unit, const char* s) {
        fprintf(stderr, "Parse Error In Line %d\n", yylineno);
    }
}

%union {
    char *str;
    struct TranslationUnit *unit;
    struct PreProcessorExpr *expr;
}

%token PP_ONE PP_TWO INCLUDE INCLUDE_NEXT LINE DEFINE IF ELSE ELIF ENDIF IFDEF IFNDEF UNDEF PRAGMA WARNING ERROR
%token DOUBLE_QUOTE LT GT STRING_LITERAL LTGT_LITERAL

%type<unit> translation_unit start
%type<expr> preprocessor_expr

// Grammar rules

%%

start : translation_unit {
    $$ = $1;
}
;

translation_unit
    : preprocessor_expr {
        *unit = new TranslationUnit;
        if (dynamic_cast<IncludeExpr *>($1)) {

        } else if (dynamic_cast<IncludeNextExpr *>($1)) {

        } else if (dynamic_cast<DefineExpr *>($1)) {

        } else if (dynamic_cast<SourceExpr *>($1)) {

        } else {

        }
        $$ = *unit;
    }
    | translation_unit preprocessor_expr {
        $$ = $1;
    }

preprocessor_expr
    : include_expr {

    }
    | include_next_expr {

    }
    | source_expr {

    }

source_expr
    : {

    }

include_expr
    : INCLUDE STRING_LITERAL {
        printf("include_expr %s\n", yylval.str);
    }
    | INCLUDE LTGT_LITERAL {
        printf("include_expr %s\n", yylval.str);
    }

include_next_expr
    : INCLUDE_NEXT STRING_LITERAL {
        
    }
    | INCLUDE_NEXT LTGT_LITERAL {

    }
%%